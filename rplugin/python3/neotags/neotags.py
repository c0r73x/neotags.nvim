import gzip
import os
import re
import subprocess
import sys
import time
from copy import copy, deepcopy
from neovim.api.nvim import NvimError
from .utils import (bindex, try_dict_get, do_set_base, do_remove_base,
                    SimpleTimer, BadFiletype, BufferChanged)

PATHSEP = '\\' if sys.platform == 'win32' else '/'
PACKAGE_NAME = "neotags"
PACKAGE_VERSION = "2.0.0a"
whitelist = [
    "cpp", "c", "go", "javascript", "java", "perl", "php", "python", "ruby",
    "rust", "sh", "vim", "zsh"
]
vim = None
settings = None


def echo(mes, force=False):
    """A true monument to laziness."""
    mes = '%s: %s\n' % (PACKAGE_NAME, mes)
    if force or settings.verbose:
        vim.out_write(mes)
    with open('/home/bml/why.log', 'ab') as fp:
        fp.write(mes.encode('utf-8', errors='replace'))


def error(mes):
    mes = "%s (ERROR): %s\n" % (PACKAGE_NAME, mes)
    vim.err_write(mes)
    with open('/home/bml/why.log', 'ab') as fp:
        fp.write(mes.encode('utf-8', errors='replace'))


def vimvar(varname, SET=None, NS=False, EV=False):
    """Either return a nvim variable prepended with 'neotags_', or set that
    variable to the value of SET and then return that value.
    """
    prefix = 'neotags#' if NS else 'neotags_'

    try:
        if EV:
            assert (SET is None)
            return vim.api.eval(prefix + varname)
        else:
            if SET is None:
                return vim.vars[prefix + varname]
            else:
                vim.vars[prefix + varname] = SET
                return SET
    except (NvimError, KeyError) as err:
        return None


class Settings():
    """Cached user settings"""

    def __init__(self):
        global settings
        if settings is not None:
            raise RuntimeError("Cannot initialize twice")
        self.binary = vimvar('bin')
        self.compression_type = vimvar('compression_type')
        self.ctags_args = vimvar('ctags_args')
        self.ctags_bin = vimvar('ctags_bin')
        self.ctags_timeout = vimvar('ctags_timeout')
        self.enabled = vimvar('enabled')
        self.find_tool = vimvar('find_tool')
        self.global_notin = vimvar('global_notin')
        self.ignore = vimvar('ignore')
        self.ignored_dirs = vimvar('ignored_dirs')
        self.recursive = vimvar('recursive')
        self.restored_groups = vimvar('restored_groups')
        self.settings_file = vimvar('settings_file')
        self.should_run_ctags = vimvar('run_ctags')
        self.silent_timeout = vimvar('silent_timeout')
        self.strip_comments = vimvar('strip_comments')
        self.use_binary = vimvar('use_binary')
        self.use_compression = vimvar('use_compression')
        self.usebin = vimvar('use_binary')
        self.verbose = vimvar('verbose')

        self.suffix = ".tags.gz" if self.use_compression else ".tags"
        self.neotags_bin = self._get_binary()
        self.__ctov = vimvar('ft_conv')
        self.__vtoc = {y: x for x, y in self.__ctov.items()}
        self.__ignored_tags = vimvar('ignored_tags')

    def setvar(self, var, new):
        setattr(self, var, new)
        vimvar(var, SET=new)

    def ignored_tags(self, ft):
        return try_dict_get(self.__ignored_tags, ft)

    def ctags_name(self, vim_name):
        try:
            return self.__vtoc[vim_name]
        except KeyError:
            return vim_name

    def _get_binary(self):
        """Try to find the neotags binary, if possible."""
        binary = self.binary
        if sys.platform == 'win32' and binary.find('.exe') < 0:
            binary += '.exe'
        if os.path.exists(binary):
            self.setvar('use_binary', 1)
        else:
            self.setvar('use_binary', 0)
            binary = None
            if self.use_binary:
                error("Binary '%s' doesn't exist. Cannot enable." % binary)

        return binary


################################################################################


class Neotags(object):
    """Main show runner"""
    notin_pattern = r'syntax match %s /%s\%%(%s\)%s/ containedin=ALLBUT,%s display'
    match_pattern_not = r'syntax match %s /%s\%%(%s\)%s/ containedin=ALLBUT,%s display'
    match_pattern = r'syntax match %s /%s\%%(%s\)%s/ display'
    keyword_pattern = r'syntax keyword %s %s'

    def __init__(self, nvim_obj):
        global vim
        vim = nvim_obj
        self.__buflist = {}
        self.__initialized = False
        self.__enabled = True

    def start(self):
        global settings
        settings = Settings()
        self.__initialized = True
        self.__enabled = settings.enabled
        self.update(True)

    def update(self, force):
        if not self.__initialized or not self.__enabled:
            return
        # Behind the scenes, every seeminly free access of these "variables" is
        # really an RPC call to neovim for the information. Best to keep them
        # to a minimum if possible.
        stime = SimpleTimer()
        buf = vim.current.buffer
        bufnum = buf.number
        if bufnum in self.__buflist:
            bdata = self.__buflist[bufnum]
        else:
            try:
                bdata = Bufdata(
                    buf, vim.request('nvim_buf_get_option', bufnum, 'ft'))
                self.__buflist[bdata.num] = bdata
            except BadFiletype:
                return

        ctick = vim.request('nvim_buf_get_changedtick', bufnum)
        if bdata.ctick != ctick or force:
            bdata.run_ctags()
            self._parse_tags(bdata)
            bdata.highlight(True)
        else:
            bdata.highlight(False)

        echo('Finished all => (%.4fs)' % (time.time() - stime.start))

    def _parse_tags(self, bdata):
        bdata.update_contents()
        bdata.parse_raw_tags()

    def setBase(self, args):
        do_set_base(echo, error, settings.settings_file, args)

    def removeBase(self, args):
        do_remove_base(echo, error, settings.settings_file, args)

    def toggle(self):
        bufnum = vim.current.buffer.number
        if self.__enabled:
            echo("Disabled", force=True)
            if bufnum in self.__buflist:
                self.__buflist[bufnum].clear()
            self.__enabled = False
        else:
            echo("Enabled", force=True)
            self.__enabled = True
            self.update(True)


################################################################################


class Bufdata():
    """Wrapper for a neovim buffer."""

    class Name():
        """POD class to house the name, basename, and dirname of a file"""

        def __init__(self, raw):
            self.full = os.path.realpath(raw)
            self.path = os.path.dirname(self.full)
            self.base = os.path.basename(self.full)

    class Group():
        default_prefix = r'\C\<'
        default_suffix = r'\>'

        def __init__(self, bdata, kind):
            self.kind = kind
            self.grpid = vimvar('%s#%c.group' % (bdata.ft, kind), NS=True, EV=True)
            pre = vimvar('%s#%c.prefix' % (bdata.ft, kind), NS=True, EV=True)
            suf = vimvar('%s#%c.suffix' % (bdata.ft, kind), NS=True, EV=True)
            self.prefix = pre if (pre is not None) else self.default_prefix
            self.suffix = suf if (suf is not None) else self.default_suffix
            self.filt_prefix = vimvar('%s#%c.filter.prefix' % (bdata.ft, kind), NS=True, EV=True)
            self.filt_suffix = vimvar('%s#%c.filter.suffix' % (bdata.ft, kind), NS=True, EV=True)
            self.notin = vimvar('%s#%s.notin' % (bdata.ft, kind), NS=True, EV=True)
            self.key = '_Neotags#%s#%c' % (bdata.ft, kind)

    def __init__(self, nvim_buffer, ft):
        if ft not in whitelist:
            raise BadFiletype
        self.buf = nvim_buffer
        self.name = Bufdata.Name(self.buf.name)
        self.ft = ft
        self.num = self.buf.number

        echo("Initializing buffer %d (ft: %s), file -> %s" %
             (self.num, self.ft, self.name.full))

        self.ctick = 0
        self.raw_tag_file = ''
        self.contents = ''
        self.ctags_ft = settings.ctags_name(self.ft)
        self.recurse = (settings.recursive
                        and self.name.path not in settings.ignored_dirs)
        self.gzfile, self.tmpfile, self.run = self._get_ctags_info()

        self.__ctags_command = self._get_ctags_command()
        self.__restore_cmd = self._get_restore_command()
        self.__processed_tags = {}
        self.__cached_command = None
        self.__ignored_tags = settings.ignored_tags(self.ft)
        self.__order = vimvar('%s#order' % self.ft, NS=True)
        self.__equiv = vimvar('%s#equivalent' % self.ft, NS=True)
        self.__groups = {
            "%s#%s" % (self.ft, kind): set()
            for kind in [chr(i) for i in self.__order.encode('utf-8')]
        }

        self.__restore_cmd = self._get_restore_command()
        self.__group_info = {}
        for kind in self.__order:
            self.__group_info[kind] = Bufdata.Group(self, kind)

    def check_ctick(self):
        """Check if the current changed tick of a buffer is different from the
        stored one. If so, the buffer has been updated.
        """
        ctick = vim.request('nvim_buf_get_changedtick', self.num)
        return self.ctick != ctick

    def update_contents(self):
        self.contents = '\n'.join(self.buf)

    def run_ctags(self):
        """Actually run the ctags command."""
        self.ctick = vim.request('nvim_buf_get_changedtick', self.num)

        proc = subprocess.Popen(
            (self.__ctags_command),
            executable=settings.ctags_bin,
            stderr=subprocess.PIPE,
            shell=False)

        proc.wait(settings.ctags_timeout)
        err = proc.communicate()[1]

        if err:
            error('Ctags completed with errors.')
            for e in err.decode('utf-8').split('\n'):
                error(e)

        try:
            with open(self.tmpfile, 'rb') as tmp:
                self.raw_tag_file = tmp.read()
                with gzip.open(self.gzfile, 'wb') as destfile:
                    destfile.write(self.raw_tag_file)
        except IOError as err:
            error('Unexpected IO Error when writing compressed file:')
            error(str(err), fatal=True)

    def _get_ctags_info(self):
        """Calculates the name of the cache file for the compressed ctags
        output and gets a temporary filename to store the uncompressed output,
        which can be used by neovim.
        """
        projects = {}
        path = self.name.path
        run = 1
        if self.recurse:
            try:
                with open(settings.settings_file, 'rb') as setfp:
                    projects = {
                        p: int(run)
                        for p, run in
                        [j.split('\t') for j in [i.rstrip() for i in setfp]]
                    }
            except FileNotFoundError:
                with open(settings.settings_file, 'xb') as setfp:
                    setfp.write('')
            for proj in projects:
                if os.path.commonpath([path, proj]) == proj:
                    path = proj
                    run = projects[path]
                    break
            path = os.path.realpath(path)
            gzfile = self._get_gzfile(path)
        else:
            gzfile = self._get_gzfile(self.name.full)

        tmpfile = vim.call('tempname')
        vim.command('set tags+=%s' % tmpfile, async_=True)
        return gzfile, tmpfile, run

    def _get_gzfile(self, path):
        """Do the simple substitution of path separation characters with two
        underscores to get the compressed filename.
        """
        if (sys.platform == 'win32'):
            path = re.sub(':', '__', path)
        gzfile = "%s/%s.tags" % (vimvar('directory'),
                                 path.replace(PATHSEP, '__'))
        return gzfile + settings.suffix

    def _get_restore_command(self):
        if self.ft not in settings.restored_groups:
            return None
        data = settings.restored_groups[self.ft]
        cmds = []

        for group in data:
            lnk, symbols = group[0], group[1]
            cmds.append('silent! syntax clear %s' % group)
            cmds.append('syntax keyword %s %s' % (group, ' '.join(symbols)))
            cmds.append('hi! link %s %s' % (group, lnk))

        return ' | '.join(cmds)

    def _get_ctags_command(self):
        """Determine the arguments which should be passed to ctags."""
        ctags_args = copy(settings.ctags_args)
        ctags_args.insert(0, '-f%s' % self.tmpfile)
        ctags_args.insert(0, '--verbose')

        if self.recurse:
            ctags_args.append('-R')
            ctags_args.append(self.name.path)
        else:
            ctags_args.append(self.name.full)

        echo('Running command \"%s\"' % ' '.join(ctags_args))
        return ctags_args

    def parse_raw_tags(self):
        echo("=============== Executing Python code ===============")
        match_list = self._do_parse_raw_file(self.raw_tag_file.split(b'\n'))
        buf = bytes(self.contents, 'utf-8', errors='replace')
        toks = sorted(list(set(re.split(b'\W', buf))))
        groups = deepcopy(self.__groups)

        for match in match_list:
            if (bindex(toks, match['name']) != (-1) or b'$' in match['name']
                    or b'.' in match['name']):
                key = "%s#%s" % (self.ft, match['kind'].decode('utf-8'))
                groups[key].add(match['name'])
        for grp in groups.keys():
            groups[grp] = list(set(groups[grp]))

        self.__processed_tags = groups

    def _do_parse_raw_file(self, lines):
        ret = []
        key_lang = self.ctags_ft.lower()

        for line in lines:
            line = line.split(b'\t')
            if len(line) <= 3 or line[0] == b'!':
                continue
            entry = {}
            entry['name'] = line[0]
            for field in line[3:]:
                if len(field) == 1:
                    entry['kind'] = field
                elif field.startswith(b'language:'):
                    entry['lang'] = field[9:]

            def contains(a, x):
                return all(map(lambda y: y in a, x))

            if contains(entry, ('lang', 'kind')):
                grp = entry['kind'].decode('utf-8')
                grp = (grp if self.__equiv is None else
                       grp if grp not in self.__equiv else
                       self.__equiv[grp])

                entry['kind'] = bytes(grp, 'utf-8')
                entry_lang = entry['lang'].decode(
                    'utf-8', errors='replace').lower()
                entry_name = entry['name'].decode('utf-8', errors='replace')

                if (grp in self.__order
                        and (key_lang == entry_lang
                             or (key_lang in ('c', 'c\\+\\+') and
                                 entry_lang in ('c', 'c++')))
                        and (self.__ignored_tags is None
                             or entry_name not in self.__ignored_tags)):
                    ret.append(entry)
            else:
                error("No lang/kind")

        return sorted(ret, key=lambda x: x['name'])

    def highlight(self, force):
        echo('Highlighting for buffer number %d' % self.num)
        command = []
        echo(str(self.__processed_tags))
        for kind in self.__order:
            key = '%s#%c' % (self.ft, kind)
            grp = self.__group_info[kind]
            if grp is not None and key in self.__processed_tags:
                tags = self.__processed_tags[key]
                command.append(
                    self._get_highlight_command_for_group(
                        grp, kind, tags, force))

        self.__cached_command = copy(command)
        if self.__restore_cmd is not None:
            command.append(self.__restore_cmd)

        atomic_args = [['nvim_command', [cmd]] for cmd in command]
        vim.request('nvim_call_atomic', atomic_args)

    def _get_highlight_command_for_group(self, grp, kind, tags, force):
        hlkey = '_Neotags_%s_%c_%s' % (self.ft, kind, grp.grpid)
        tags = [b.decode() for b in tags]

        if (not force and self.__cached_command is not None
                and kind in self.__cached_command):
            command = self.__cached_command[kind]
        else:
            command = 'silent! syntax clear %s | ' % hlkey
            join1 = r' '.join(tags)
            if grp.notin:
                notin = [*grp.notin, *settings.global_notin]
                command += Neotags.notin_pattern % (
                    hlkey, grp.prefix, r'\|'.join(tags), grp.suffix,
                    ','.join(notin))
            elif (grp.prefix == self.Group.default_prefix
                  and grp.suffix == self.Group.default_suffix
                  and '.' not in join1):
                command += Neotags.keyword_pattern % (hlkey, join1)
            else:
                command += (Neotags.match_pattern_not %
                            (hlkey, grp.prefix, r'\|'.join(tags), grp.suffix,
                             ','.join(settings.global_notin)))

            command += ' | hi def link %s %s' % (hlkey, grp.grpid)

        if vim.current.buffer != self.buf:
            error("Buffer changed mid processing, aborting command.")
            raise BufferChanged

        return command

    def clear(self):
        cmds = []
        for grp in self.__group_info.values():
            cmds.append('silent! syntax clear %s_%s' % (grp.key.replace('#', '_'), grp.grpid))
        echo(cmds)
        vim.command(' | '.join(cmds), async_=True)
