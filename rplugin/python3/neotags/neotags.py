# ============================================================================
# File:        neotags.py
# Author:      Christian Persson <c0r73x@gmail.com>
# Repository:  https://github.com/c0r73x/neotags.nvim
#              Released under the MIT license
# ============================================================================

# NOTE: Psutil import has been moved to Neotags._kill() to allow the script to
# run without the module (and therefore essentially without the kill function;
# beggers can't be choosers). Psutil is not and will never be available on
# Cygwin, which makes this plugin unusable there without this change.

import hashlib
import inspect
import os
import re
import subprocess
import time
import atexit
from sys import platform
from tempfile import NamedTemporaryFile
from neovim.api.nvim import NvimError
from copy import deepcopy

CLIB = None
if platform == 'win32':
    SEPCHAR = ';'
    DELETE = False
else:
    SEPCHAR = ':'
    DELETE = True


class Neotags(object):
    def __init__(self, vim):
        self.__prefix = r'\C\<'
        self.__suffix = r'\>'
        self.__initialized = False
        self.__is_running = False
        self.__run_ctags = False

        self.__cmd_cache = {}
        self.__exists_buffer = {}
        self.__groups = {}
        self.__md5_cache = {}
        self.__regex_buffer = {}
        self.__tmp_cache = {}

        self.__seen = []
        self.__start_time = []
        self.__backup = []

        self.__autocmd = None
        self.__gzfile = None
        self.__init_tagfiles = None
        self.__neotags_bin = None
        self.__patternlength = None
        self.__slurp = None
        self.__tagfile = None

        self.__ctov = ''
        self.__fsuffix = ''
        self.__keyword_pattern = ''
        self.__match_pattern = ''
        self.__notin_pattern = ''
        self.__to_escape = ''
        self.__vtoc = ''

        self.__globtime = time.time()
        self.__hlbuf = 1

        self.vim = vim
        
        #if platform == 'win32':
        #    atexit.register(cleanup_tmpfiles, self.__tmp_cache)

    def __void(self, *_, **__):
        return

    def init(self):
        if self.__initialized:
            return
        self.__ctov = self.vv('ft_conv')
        self.__init_tagfiles = self.vim.api.eval('&tags').split(",")
        self.__to_escape = re.compile(r'[.*^$/\\~\[\]]')
        self.__vtoc = {y: x for x, y in self.__ctov.items()}

        self.__notin_pattern = r'syntax match %s /%s\%%(%s\)%s/ containedin=ALLBUT,%s'
        self.__match_pattern = r'syntax match %s /%s\%%(%s\)%s/'
        self.__keyword_pattern = r'syntax keyword %s %s'

        if self.vv('use_binary') == 1:
            self.__neotags_bin = self._get_binary()
        self.__backup = [self._debug_echo, self._debug_start, self._debug_end]

        if not self.vv('verbose'):
            self._debug_start = self.__void
            self._debug_echo = self.__void
            self._debug_end = self.__void

        if self.vv('use_compression'):
            global CLIB
            ctype = self.vv('compression_type')

            if ctype in ('gz', 'gzip'):
                import gzip as CLIB
                self.vv('compression_type', SET='gzip')
                self.__fsuffix = '.gz'
            elif ctype in ('xz', 'lzma'):
                import lzma as CLIB
                self.vv('compression_type', SET='lzma')
                self.__fsuffix = '.xz'
            else:
                self._error("Neotags: Unrecognized compression type.")
                self.vv('compression_type', SET=None)
                self.vv('use_compression', SET=0)

        self._debug_echo("Using compression type %s with ext %s" %
                         (self.vv('compression_type'), self.__fsuffix), pop=False)

        self.__autocmd = "if execute('autocmd User') =~# 'NeotagsPost' | doautocmd User NeotagsPost | endif"
        self.__initialized = True

        if self.vv('enabled'):
            evupd = ','.join(self.vv('events_update'))
            evhl = ','.join(self.vv('events_highlight'))
            evre = ','.join(self.vv('events_rehighlight'))

            self.__patternlength = self.vv('patternlength')

            self.vim.command('autocmd %s * call NeotagsUpdate()' % evupd, async=True)
            self.vim.command('autocmd %s * call NeotagsHighlight()' % evhl, async=True)
            self.vim.command('autocmd %s * call NeotagsRehighlight()' % evre, async=True)

            if self.vv('loaded'):
                self.update(False)

    def update(self, force=False):
        """Update tags file, tags cache, and highlighting."""
        ft = self.vim.api.eval('&ft')
        if not self.vv('enabled'):
            self._debug_echo('Update called when plugin disabled...', pop=False)
            self._clear(ft)
            self.vim.command(self.__autocmd, async=False)
            return

        if (ft == '') or (ft in self.vv('ignore')):
            self.vim.command(self.__autocmd, async=False)
            return

        if self.__is_running:
            return
        self.__is_running = True

        self._update(ft, force)
        self.__is_running = False
        self.highlight(False)

        self.vim.command(self.__autocmd, async=False)

    def highlight(self, clear):
        """Analyze the tags data and format it for nvim's regex engine."""
        self.__globtime = time.time()
        self.__exists_buffer = {}
        hl = HighlightGroup()
        hl.ft = self.vim.api.eval('&ft')
        force = clear

        if clear:
            self._clear(hl.ft)
        if not self.vv('enabled') or not self.vv('highlight'):
            self._clear(hl.ft)
            return
        elif hl.ft == '' or hl.ft in self.vv('ignore') or self.__is_running:
            return
        else:
            self.__is_running = True

        self._debug_start()
        hl.file = self.vim.api.eval("expand('%:p:p')")
        order = self._tags_order(hl.ft)
        groups = self.__groups[hl.ft]

        if groups is None:
            self.__is_running = False
            self._debug_end('Skipping file')
            return

        if not order:
            order = groups.keys()

        # self._debug_echo("order: %s, groups:%s" % (order, groups))
        for hl.key in order:
            hl.group = self._exists(hl.key, '.group', None)
            fgroup = self._exists(hl.key, '.filter.group', None)

            if hl.group is not None and hl.key in groups:
                hl.prefix = self._exists(hl.key, '.prefix', self.__prefix)
                hl.suffix = self._exists(hl.key, '.suffix', self.__suffix)
                hl.notin = self._exists(hl.key, '.notin', [])

                if not self._highlight(hl, groups[hl.key], force):
                    break

            fkey = hl.key + '_filter'
            if fgroup is not None and fkey in groups:
                fhl = deepcopy(hl)
                fhl.key = fkey
                fhl.prefix = self._exists(hl.key, '.filter.prefix', self.__prefix)
                fhl.suffix = self._exists(hl.key, '.filter.suffix', self.__suffix)
                fhl.notin = self._exists(hl.key, '.filter.notin', [])

                if not self._highlight(fhl, groups[fhl.key], force):
                    break

        self._debug_end('applied syntax for %s' % hl.ft)

        while self.__start_time:
            self._debug_end("Value:")
            self._error("Extra value in self.__start_time...")

        self.__hlbuf = self.vim.current.buffer.number
        self.__is_running = False

##############################################################################
# Projects

    def setBase(self, args):
        try:
            with open(self.vv('settings_file'), 'r') as fp:
                projects = [i.rstrip for i in fp]
        except FileNotFoundError:
            projects = []

        with open(self.vv('settings_file'), 'a') as fp:
            for arg in args:
                path = os.path.realpath(arg)
                if os.path.exists(path):
                    if path not in projects:
                        fp.write(path + '\n')
                        self._inform_echo("Saved directory '%s' as a project"
                                          " base." % path)
                    else:
                        self._inform_echo("Error: directory '%s' does not"
                                          " exist." % path)
                else:
                    self._inform_echo("Error: directory '%s' is already saved"
                                      " as a project base." % path)

    def removeBase(self, args):
        try:
            with open(self.vv('settings_file'), 'r') as fp:
                projects = [i.rstrip() for i in fp]
        except FileNotFoundError:
            return
        path = os.path.realpath(args[0])

        if path in projects:
            projects.remove(path)
            self._inform_echo(
                "Removed directory '%s' from project list." % path)
            with open(self.vv('settings_file'), 'w') as fp:
                for path in projects:
                    fp.write(path + '\n')
        else:
            self._inform_echo(
                "Error: directory '%s' is not a known project base." % path)

##############################################################################
# Private

    def _update(self, ft, force=False):
        if self.vim.current.buffer.number not in self.__seen:
            self.__seen.append(self.vim.current.buffer.number)
        if self.vv('run_ctags'):
            self._run_ctags(force)
        self.__groups[ft] = self._parseTags(ft)

    def _highlight(self, hl, group, force):
        self._debug_start()
        highlights, number = self._getbufferhl()

        self._debug_echo("Highlighting for buffer %s" % number)
        if number in self.__md5_cache:
            highlights = self.__md5_cache[number]
        else:
            self.__md5_cache[number] = highlights = {}

        current = []
        cmds = []
        hl.key = '_Neotags_%s_%s' % (hl.key.replace('#', '_'), hl.group)

        md5 = hashlib.md5()
        strgrp = ''.join(group).encode('utf8')

        for i in range(0, len(strgrp), 128):
            md5.update(strgrp[i:i + 128])

        md5hash = md5.hexdigest()

        if not force \
                and (hl.key in highlights and md5hash == highlights[hl.key]) \
                or (number != self.__hlbuf and number in self.__cmd_cache
                    and hl.key in self.__cmd_cache[number]):
            try:
                cmds = self.__cmd_cache[number][hl.key]
            except KeyError:
                self._error('Key error in _highlight()!')
                self.__start_time.pop()
                return True
            self._debug_echo("Updating from cache" % cmds)

        else:
            cmds.append('silent! syntax clear %s' % hl.key)

            for i in range(0, len(group), self.__patternlength):
                current = group[i:i + self.__patternlength]
                if hl.notin:
                    cmds.append(self.__notin_pattern %
                                (hl.key, hl.prefix, r'\|'.join(current),
                                 hl.suffix, ','.join(hl.notin)))
                elif (hl.prefix == self.__prefix and
                      hl.suffix == self.__suffix):
                    cmds.append(self.__keyword_pattern %
                                (hl.key, ' '.join(current)))
                else:
                    cmds.append(self.__match_pattern % (hl.key, hl.prefix,
                                r'\|'.join(current), hl.suffix))

            if hl.ft != self.vim.api.eval('&ft'):
                self._debug_end('filetype changed aborting highlight')
                return False

            self.__md5_cache[number][hl.key] = md5hash
            cmds.append('hi link %s %s' % (hl.key, hl.group))

        full_cmd = ' | '.join(cmds)
        self.vim.command(full_cmd, async=True)

        try:
            self.__cmd_cache[number][hl.key] = cmds
        except KeyError:
            self.__cmd_cache[number] = {}
            self.__cmd_cache[number][hl.key] = cmds
        finally:
            self._debug_end('Updated highlight for %s' % hl.key)
            return True

    def _parseTags(self, ft):
        self._get_file()
        files = []

        self._debug_start()
        self._debug_echo("Using tags file %s" % self.__gzfile)

        self._debug_echo("run_ctags -> %d" % self.vv('run_ctags'))
        if not os.path.exists(self.__gzfile):
            if self.vv('run_ctags'):
                self._debug_echo("Tags file does not exist. Running ctags.")
                self._run_ctags(True)
                files.append(self.__gzfile)
            else:
                self._debug_echo(
                    'No compressed tags file exists and not running ctags...')
                self.__gzfile = None
        else:
            self._debug_echo('updating vim-tagfile', pop=False)
            with self._open(self.__gzfile, 'rb', self.vv('compression_type')) as fp:
                self._update_vim_tagfile(self.__gzfile, fp)
            files.append(self.__gzfile)

        for File in self.__init_tagfiles:
            if os.path.exists(File):
                files.append(File)

        self._debug_end("Finished updating file list")

        if not files:
            self._error('No tag files found!')
            return None

        # Slurp the whole content of the current buffer
        self.__slurp = '\n'.join(self.vim.current.buffer)

        if self.__neotags_bin is not None:
            try:
                self._debug_echo("Using C binary to analyze tags.", pop=False)
                return self._bin_get_tags(files, ft)
            except CBinError as err:
                self.vim.command("echoerr 'C binary failed with status %d: \"%s\"' "
                                 "| echoerr 'Will try python code.'" % err.args, async=True)
                return self._get_tags(files, ft)
        else:
            self._debug_echo("Using python code to analyze tags.", pop=False)
            return self._get_tags(files, ft)

# =============================================================================
# Yes C binary

    def _bin_get_tags(self, files, ft):
        filetypes = ft.lower().split('.')
        languages = ft.lower().split('.')

        vimlang = languages[0]
        lang = self._vim_to_ctags(languages)[0]

        try:
            order = self.vim.api.eval('neotags#%s#order' % ft)
        except NvimError:
            return None
        try:
            equivalent = self.vim.api.eval('neotags#%s#equivalent' % ft)
        except NvimError:
            equivalent = None

        groups = {
            "%s#%s" % (ft, kind): []
            for kind in [chr(i) for i in order.encode('ascii')]
        }

        if filetypes is None:
            return groups

        if self.__gzfile is not None:
            comp_type = self.vv('compression_type')
            comp_type = 'none' if comp_type is None else comp_type
            file_list = '%s%s%s%s' % (comp_type, SEPCHAR, files[0], SEPCHAR)
            for File in files[1:]:
                file_list += 'none%s%s%s' % (SEPCHAR, File, SEPCHAR)
        else:
            file_list = ''
            for File in files:
                file_list += 'none%s%s%s' % (SEPCHAR, File, SEPCHAR)

        stime = time.time()
        self._debug_start()
        self._debug_echo("=============== Executing C code ===============")

        ignored_tags = self.vv('ignored_tags')
        ignored_tags = (SEPCHAR.join(ignored_tags) + SEPCHAR) if ignored_tags else ''

        if equivalent is None:
            equiv_str = ''
        else:
            equiv_str = SEPCHAR.join([A + B for A, B in equivalent.items()]) + SEPCHAR

        self._debug_echo("Cmd is: %s" % [
            self.__neotags_bin, file_list, lang, vimlang, order,
            str(self.vv('strip_comments')), str(len(self.__slurp)),
            ignored_tags, equiv_str])

        # I wrote this little program to expect its arguments to be given in a
        # precise order. Since its only use is as a filter to this script, this
        # just seemed easier than something more robust. For reference, the
        # arguments it needs are:
        #    1) List of tags files, with the compression type of the file (none,
        #       gzip, or lzma), a colon, the filename, and a terminating colon,
        #       followed by any further files.
        #    2) The language of the current buffer in ctags' format
        #    3) The same in vim's format
        #    4) The `order' string
        #    5) Whether to strip out comments (0 or 1)
        #    6) The length in bytes of the current vim buffer
        #    7) The `ignored' tags list, separated and terminated by semicolons
        #    8) The list of groups considered equivalent (semicolon separated)
        # All numbers must be converted to strings for the subprocess interface.
        proc = subprocess.Popen(
            (
                self.__neotags_bin,
                file_list,
                lang,
                vimlang,
                order,
                str(self.vv('strip_comments')),
                str(len(self.__slurp)),
                ignored_tags,
                equiv_str,
            ),
            stdin=subprocess.PIPE,
            stderr=subprocess.PIPE,
            stdout=subprocess.PIPE,
            )
        out, err = proc.communicate(input=self.__slurp.encode('utf-8', errors='replace'))
        # splitchar = '\r\n' if platform == 'win32' else '\n'
        out = out.decode(errors='replace').rstrip().split('\n')
        err = err.decode(errors='replace').rstrip().split('\n')

        self._debug_echo("Returned %d items" % (len(out) / 2))
        # for i in range(0, len(out)-1, 2):
        #     line1, line2 = out[i], out[i+1]
        #     if line1 and line2:
        #         self._debug_echo("OUT: %s - %s" % (line1, line2), pop=False)
        for line in err:
            if line:
                self._debug_echo("ERR: %s" % line, pop=False)

        if proc.returncode:
            self.__start_time.pop()
            raise CBinError(proc.returncode, err[-1])

        for i in range(0, len(out) - 1, 2):
            key = "%s#%s" % (ft, out[i].rstrip('\r'))
            try:
                groups[key].append(out[i+1].rstrip('\r'))
            except KeyError:
                groups[key] = [out[i+1].rstrip('\r')]

        self._debug_end('Done reading files: %s' % str(files))
        self._debug_echo("Elapsed time for reading file: %fs" %
                         (float(time.time()) - stime), err=True)

        # with open(os.environ['HOME']+'/cbin.log', 'w') as fp:
        #     a = '\n'.join([str(i) for s in groups.values() for i in s])
        #     print(a, file=fp)
        return groups

# =============================================================================
# No C binary

    def _get_tags(self, files, ft):
        filetypes = ft.lower().split('.')
        languages = ft.lower().split('.')
        groups = {}
        ignored_tags = self.vv('ignored_tags')

        try:
            order = self.vim.api.eval('neotags#%s#order' % ft)
        except NvimError:
            return
        try:
            equivalent = self.vim.api.eval('neotags#%s#equivalent' % ft)
        except NvimError:
            equivalent = None

        stime = time.time()

        groups = {
            "%s#%s" % (ft, kind): []
            for kind in [chr(i) for i in order.encode('ascii')]
        }

        if filetypes is None:
            return groups

        if self.__gzfile is None:
            comp_type = None
        else:
            comp_type = self.vv('compression_type')

        lang = '|'.join(self._vim_to_ctags(filetypes))

        pattern = re.compile(
            b'(?:^|\n)(?P<name>[^\t]+)\t(?:[^\t]+)\t\/(?:.+)\/;"\t(?P<kind>\w)\tlanguage:(?P<lang>'
            + bytes(lang, 'utf8') + b')', re.IGNORECASE)

        for File in files:
            with self._open(File, 'rb', comp_type) as fp:
                data = fp.read()
                match_list = pattern.finditer(data)

            self.parse(ft, match_list, groups, languages, ignored_tags, equivalent, order)
            comp_type = None

        # with open(os.env['HOME']+'/python.log', 'w') as fp:
        #     a = '\n'.join([str(i) for s in groups.values() for i in s])
        #     print(a, file=fp)

        self._debug_echo("Elapsed time for reading file: %fs" %
                         (float(time.time()) - stime), err=True)
        return groups

    def parse(self, ft, match_list, groups, languages, ignored_tags,
              equivalent, order):
        self._debug_start()
        self._debug_echo("=============== Executing Python code ===============")
        key_lang = languages[0]

        self._debug_echo("equivalent: %s" % equivalent)

        for match in match_list:
            match = { a: b.decode() for a, b in match.groupdict().items() }
            kind = match['kind']

            # I tried to do this in the least Pythonic way possible
            grp = kind if equivalent is None else \
                  kind if kind not in equivalent else \
                  equivalent[kind]
            key = "%s#%s" % (ft, grp)

            match_lang = self._ctags_to_vim(match['lang'], languages)

            # Skip tags that are:
            #     1) Of a type not present in the 'order' string
            #     2) The wrong language (C and C++ are considered equivalent)
            #     3) In the user specified ignore list
            #     4) Duplicates
            #     5) Not present in the current vim buffer.
            # Sometimes 'key' is not yet set in the 'groups' dict, leading to
            # an IndexError. It is cheaper to cheaper to wrap this in a try
            # block than to check for that every time.
            try:
                if ((order.find(grp) == (-1))
                        or (key_lang != match_lang
                            and key_lang   not in ('c', 'cpp')
                            and match_lang not in ('c', 'cpp'))
                        or (match['name'] in ignored_tags)
                        or (match['name'] in groups[key])):  # <- duplicates
                    continue
            except IndexError:
                pass
            if self.__slurp.find(match['name']) == (-1):
                continue

            groups[key].append(match['name'])

        self._debug_end("Finished parse, found %d items." % sum(map(len, groups.values())))

# =============================================================================

    def _run_ctags(self, force):
        self._debug_start()
        ctags_command = self._get_ctags_command(force)
        if ctags_command is None:
            self._debug_end("Not running ctags.")
            return

        try:
            proc = subprocess.Popen(
                ctags_command, shell=True, stderr=subprocess.PIPE)
            proc.wait(self.vv('ctags_timeout'))
            err = proc.communicate()[1]

            if err:
                self._error('Ctags completed with errors')
                for e in err.decode('ascii').split('\n'):
                    self._error(e)
            else:
                self._debug_echo('Ctags completed successfully')

            cmpt = self.vv('compression_type')
            try:
                self._debug_start()
                with open(self.__tagfile, 'rb') as src:
                    with self._open(self.__gzfile, 'wb', cmpt, level=9) as dst:
                        dst.write(src.read())
                    src.seek(0)
                    self._update_vim_tagfile(self.__gzfile, src)

                os.unlink(self.__tagfile)

            except IOError as err:
                self._error("Unexpected IO Error -> '%s'" % err)

            finally:
                self._debug_end('Finished compressing file.')

        except FileNotFoundError as error:
            self._error('failed to run Ctags %s' % error)

        except subprocess.TimeoutExpired:
            try:
                self._kill(proc.pid)
            except ImportError:
                proc.kill()
            else:
                if self.vv('silent_timeout') == 0:
                    self.vim.command("echom 'Ctags process timed out!'",
                                     async=True)

        finally:
            self._debug_end("Finished running ctags")

    def _get_ctags_command(self, force):
        ctags_args = self.vv('ctags_args')

        # NOTE: _get_file() sets self.__tagfile and self.__gzfile!
        recurse, path = self._get_file()
        if not force and os.path.exists(self.__gzfile) \
                     and os.stat(self.__gzfile).st_size > 0:
            return None

        ctags_args.append('-f "%s"' % self.__tagfile)
        ctags_binary = None

        if recurse:
            if self.vv('find_tool'):
                ctags_args.append('-L-')
                ctags_binary = "%s %s | %s" % (
                    self.vv('find_tool'), path,
                    self.vv('ctags_bin'))
                self._debug_echo(
                    "Using %s to find files recursively in dir '%s'" %
                    (self.vv('find_tool'), path))
            else:
                ctags_args.append('-R')
                ctags_args.append('"%s"' % path)
                ctags_binary = self.vv('ctags_bin')

                self._debug_echo("Running ctags on dir '%s'" % path)

        else:
            self._debug_echo(
                "Not running ctags recursively for dir '%s'" % path)
            File = os.path.realpath(self.vim.api.eval("expand('%:p')"))
            ctags_args.append('"%s"' % File)
            ctags_binary = self.vv('ctags_bin')

            self._debug_echo("Running ctags on file '%s'" % File)

        full_command = '%s %s' % (ctags_binary, ' '.join(ctags_args))
        self._debug_echo(full_command)

        return full_command

# ==============================================================================
# Debug and util

    def _tags_order(self, ft):
        orderlist = []
        filetypes = ft.lower().split('.')

        for filetype in filetypes:
            order = self._exists(filetype, '#order', None)
            if order:
                orderlist += [(filetype + '#') + s for s in list(order)]

        return orderlist

    def _exists(self, kind, var, default):
        buf = kind + var

        if buf not in self.__exists_buffer:
            try:
                self.__exists_buffer[buf] = self.vim.api.eval('neotags#' + buf)
            except NvimError:
                self.__exists_buffer[buf] = default

        return self.__exists_buffer[buf]

    def _getbufferhl(self):
        number = self.vim.current.buffer.number

        if number in self.__md5_cache.keys():
            highlights = self.__md5_cache[number]
        else:
            self.__md5_cache[number] = highlights = {}

        return highlights, number

    def _clear(self, ft):
        if ft is None:
            self._debug_echo('Clear called with null ft')
            return

        highlights, _ = self._getbufferhl()
        cmds = []
        order = self._tags_order(ft)

        for key in order:
            hlgroup = self._exists(key, '.group', None)
            hlkey = '_Neotags_%s_%s' % (key.replace('#', '_'), hlgroup)
            cmds.append('silent! syntax clear %s' % hlkey)

        self._debug_echo(str(cmds), pop=False)

        self.vim.command(' | '.join(cmds), async=True)

    def _debug_start(self):
        self.__start_time.append(time.time())

    def _debug_echo(self, message, pop=False, err=False):
        if pop:
            elapsed = time.time() - self.__start_time[-1]
            self.vim.command(
                'echom "%s (%.2fs) -" "%s"' %
                (self.__to_escape.sub(r'\\\g<0>', message).replace('"', r'\"'),
                 elapsed, self.__start_time))
            # self.vim.out_write('%s (%.2fs) - %s' % (message, elapsed, self.__start_time))
        elif err:
            self._error(message)
        else:
            self._inform_echo(message)

    def _debug_end(self, message):
        otime = self.__start_time.pop()
        self._debug_echo('%d: (%.4fs) END   => %s' % (inspect.stack()[1][2],
                         time.time() - otime, message))

    def _inform_echo(self, message):
        self.vim.command('echom "%s"' % self.__to_escape.sub(
                         r'\\\g<0>', message).replace('"', r'\"'))
        # self.vim.out_write(message)

    def _error(self, message):
        if message:
            message = 'Neotags: ' + message
            message = message.replace('\\', '\\\\').replace('"', '\\"')
            self.vim.command('echohl ErrorMsg | echom "%s" | echohl None' %
                             message)
            # self.vim.err_write(message)

    def _kill(self, proc_pid):
        import psutil

        process = psutil.Process(proc_pid)
        for proc in process.children():
            proc.kill()
        process.kill()

    def _ctags_to_vim(self, lang, languages):
        lang = lang.strip('\\')
        if lang in self.__ctov and self.__ctov[lang] in languages:
            return self.__ctov[lang]

        return lang.lower()

    def _vim_to_ctags(self, languages):
        for i, lang in enumerate(languages):

            if lang in self.__vtoc:
                languages[i] = self.__vtoc[lang]
                languages[i] = languages[i].strip('\\')

            languages[i] = re.escape(languages[i])

        return languages

    def _get_file(self):
        File = os.path.realpath(self.vim.api.eval("expand('%:p')"))
        path = os.path.dirname(File)
        projects = []

        self._debug_start()

        recurse = (self.vv('recursive')
                   and path not in self.vv('norecurse_dirs'))

        if recurse:
            try:
                with open(self.vv('settings_file'), 'r') as fp:
                    projects = [i.rstrip() for i in fp]
            except FileNotFoundError:
                with open(self.vv('settings_file'), 'x') as fp:
                    fp.write('')
                projects = []

            for proj_path in projects:
                if os.path.commonpath([path, proj_path]) == proj_path:
                    path = proj_path
                    break

            path = os.path.realpath(path)
            self._path_replace(path)

        else:
            self._path_replace(File)

        self.vim.command(
            'let g:neotags_file = "%s"' % self.__tagfile, async=True)
        self._debug_end("File is '%s'" % self.__tagfile)

        return recurse, path

    def _path_replace(self, path):
        if (platform == 'win32'):
            # For some reason replace wouldn't work here. I have no idea why.
            path = re.sub(':', '__', path)
            sep_char = '\\'
        else:
            sep_char = '/'

        self.__tagfile = "%s/%s.tags" % (self.vv('directory'),
                                         path.replace(sep_char, '__'))
        self.__gzfile = self.__tagfile + self.__fsuffix

    def _get_binary(self, loud=False):
        binary = self.vv('bin')

        if platform == 'win32' and binary.find('.exe') < 0:
            binary += '.exe'

        if os.path.exists(binary):
            self.vv('use_binary', SET=1)
        else:
            self.vv('use_binary', SET=0)
            binary = None
            if loud:
                self._inform_echo("Binary '%s' doesn't exist. Cannot enable." %
                                  self.__neotags_bin)
            else:
                self._debug_echo(
                    "Binary '%s' doesn't exist." % self.__neotags_bin, pop=False)

        return binary

    def _update_vim_tagfile(self, tagfile, open_file):
        try:
            if tagfile not in self.__tmp_cache:
                if platform == 'win32':
                    tmpfile = open(self.vim.call('tempname'), 'wb')
                else:
                    tmpfile = NamedTemporaryFile(prefix="neotags", delete=DELETE)

                self.__tmp_cache[tagfile] = {
                    'fp': tmpfile,
                    'name': tmpfile.name
                }
                tmpfile.write(open_file.read())
                self.vim.command('set tags+=%s' % tmpfile.name, async=True)
            else:
                if platform == 'win32':
                    tmpfile = open(self.__tmp_cache[tagfile]['name'], 'wb')
                else:
                    tmpfile = self.__tmp_cache[tagfile]['fp']
                tmpfile.seek(0)
                tmpfile.truncate(0)
                tmpfile.flush()
                tmpfile.write(open_file.read())

            tmpfile.flush()
            if platform == 'win32':
                tmpfile.close()

        except IOError as err:
            self._error("Unexpected io error: %s" % err)

    def _open(self, filename, mode, comp_type, level=None, **kwargs):
        if comp_type is None:
            string = 'open(filename, mode, **kwargs)'

        elif comp_type == 'gzip':
            string = 'CLIB.open(filename, mode, %s **kwargs)' % (
                '' if level is None else 'compresslevel=level,')

        elif comp_type == 'lzma':
            string = 'CLIB.open(filename, mode, %s **kwargs)' % (
                '' if level is None else 'preset=level,')

        return eval(string)

    def vv(self, varname, SET=None):
        """Either return a nvim variable prepended with 'neotags_', or set that
        variable to the value of SET and then return that value.
        """
        try:
            if SET is None:
                return self.vim.vars["neotags_" + varname]
            else:
                self.vim.vars["neotags_" + varname] = SET
                return SET

        except (NvimError, KeyError) as err:
            self._error("ERROR: varname %s doesn't exist." % varname)
            raise err


###############################################################################
# Toggling. These are ugly and repeditive.

    def toggle(self):
        """Toggle state of the plugin."""
        if not self.vv('enabled'):
            self._inform_echo("Re-enabling neotags.")
            self.vv('enabled', SET=1)
            self.update(force=True)
        else:
            self._inform_echo("Disabling neotags.")
            self.vv('enabled', SET=0)
            self.__seen = []
            self.__md5_cache = {}
            self.__cmd_cache = {}
            self.update(force=False)

    def toggle_C_bin(self):
        if self.__neotags_bin is None:
            self.__neotags_bin = self._get_binary(loud=True)
            if self.__neotags_bin is not None:
                self._inform_echo("Switching to use C binary.")
                self.vv('use_binary', SET=1)
        else:
            self.__neotags_bin = None
            self.vv('use_binary', SET=0)
            self._inform_echo("Switching to use python code.")

    def toggle_verbosity(self):
        if self._debug_echo == self.__void:
            self._inform_echo('Switching to verbose output.')
            (self._debug_echo, self._debug_start,
             self._debug_end) = self.__backup
            self.vv('verbose', SET=1)
        else:
            self._inform_echo('Switching off verbose output.')
            self._debug_start = self._debug_echo = self._debug_end = self.__void
            self.vv('verbose', SET=0)



class HighlightGroup:
    """Exists to keep the number of arguments being passed around down."""
    def __init__(self):
        self.file = None
        self.ft = None
        self.group = None
        self.key = None
        self.notin = None
        self.prefix = None
        self.suffix = None


class CBinError(Exception):
    """Dummy wrapper."""
