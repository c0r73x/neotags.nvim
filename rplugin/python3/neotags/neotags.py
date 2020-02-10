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
import os
import re
import subprocess
import sys
import time
from copy import deepcopy
from pynvim.api import NvimError

# sys.path.append(os.path.dirname(__file__))
from neotags.utils import (do_set_base, do_remove_base, find_tags, strip_c,
                           tokenize, bindex)
from neotags.diagnostics import Diagnostics

CLIB = None
dia = None
if sys.platform == 'win32':
    SEPCHAR = ';'
else:
    SEPCHAR = ':'


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
        self.__cur = {'file': None, 'buf': None}

        self.__seen = []
        self.__backup_groups = {}

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
        self.__match_pattern_not = ''
        self.__notin_pattern = ''
        self.__to_escape = ''
        self.__vtoc = ''

        self.__hlbuf = 1

        self.vim = vim

    def init(self):
        if self.__initialized:
            return
        self.__ctov = self.vv('ft_conv')
        self.__vtoe = self.vv('ft_ext')
        self.__init_tagfiles = self.vim.api.eval('&tags').split(",")
        self.__to_escape = re.compile(r'[.*^$/\\~\[\]]')

        self.__vtoc = {}
        for x, y in self.__ctov.items():
            if isinstance(y, list):
                for z in y:
                    self.__vtoc[z] = x
            else:
                self.__vtoc[y] = x

        self.__notin_pattern = r'syntax match %s /%s\%%(%s\)%s/ containedin=ALLBUT,%s display'
        self.__match_pattern_not = r'syntax match %s /%s\%%(%s\)%s/ containedin=ALLBUT,%s display'
        self.__match_pattern = r'syntax match %s /%s\%%(%s\)%s/ display'
        self.__keyword_pattern = r'syntax keyword %s %s'
        self.__tagfiles_by_type = self.vv('tagfiles_by_type')

        if self.vv('use_binary') == 1:
            self.__neotags_bin = self._get_binary()

        global dia
        dia = Diagnostics(bool(self.vv('verbose')), self.vim, self.vv)

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
                dia.error("Neotags: Unrecognized compression type.")
                self.vv('compression_type', SET=None)
                self.vv('use_compression', SET=0)
        else:
            self.vv('compression_type', SET=None)
            self.vv('use_compression', SET=0)

        dia.debug_echo("Using compression type %s with ext %s" %
                       (self.vv('compression_type'), self.__fsuffix))

        self.__autocmd = "if execute('autocmd User') =~# 'NeotagsPost' | " \
                         "doautocmd User NeotagsPost | endif"
        self.__initialized = True

        if self.vv('enabled'):
            evupd = ','.join(self.vv('events_update'))
            evhl = ','.join(self.vv('events_highlight'))
            evre = ','.join(self.vv('events_rehighlight'))

            self.__patternlength = self.vv('patternlength')

            self.vim.command('autocmd %s * call NeotagsUpdate()' %
                             evupd, async_=True)
            self.vim.command(
                'autocmd %s * call NeotagsHighlight()' % evhl, async_=True)
            self.vim.command(
                'autocmd %s * call NeotagsRehighlight()' % evre, async_=True)

            if self.vv('loaded'):
                self.update(False)

    def update(self, force):
        """Update tags file, tags cache, and highlighting."""
        ft = self.vim.api.eval('&ft')
        init_time = time.time()

        if not self.vv('enabled'):
            self._clear(ft)
            self.vim.command(self.__autocmd, async_=True)
            return
        if ft == '' or ft in self.vv('ignore') or self.vim.api.eval('&previewwindow'):
            self.vim.command(self.__autocmd, async_=True)
            return
        if self.__is_running:
            # XXX This should be more robust
            return

        dia.debug_start()
        self.__is_running = True
        self.__exists_buffer = {}

        hl = HighlightGroup()
        hl.ft = ft
        hl.file = self.vim.api.eval("expand('%:p:p')")
        self.__cur['file'] = os.path.realpath(hl.file)
        self.__cur['buf'] = self.vim.current.buffer
        hl.highlights, hl.number = self._getbufferhl()

        if hl.number in self.__md5_cache:
            hl.highlights = self.__md5_cache[hl.number]
        else:
            self.__md5_cache[hl.number] = hl.highlights = {}

        if force:
            if self.vv('run_ctags'):
                self._run_ctags(True)
            self.__groups[ft] = self._parseTags(ft)

        elif hl.number not in self.__seen:
            self.__seen.append(self.__cur['buf'].number)
            self.__groups[ft] = self._parseTags(ft)

        elif hl.number not in self.__cmd_cache:
            self.__groups[ft] = self._parseTags(ft)

        self.highlight(force, hl)

        self.vim.command(self.__autocmd, async_=True)

        dia.clear_stack()
        dia.debug_echo('Finished all => (%.4fs)' % (time.time() - init_time))
        self.__is_running = False

    def highlight(self, force, hl):
        """Analyze the tags data and format it for nvim's regex engine."""
        restored_groups = self.vv('restored_groups')
        if hl.ft not in self.__backup_groups:
            self.__backup_groups[hl.ft] = {}

        if hl.ft in restored_groups and restored_groups[hl.ft]:
            for group in restored_groups[hl.ft]:
                if group not in self.__backup_groups[hl.ft]:
                    self._get_backup(hl.ft, group)

        groups = self.__groups[hl.ft]
        order = self._tags_order(hl.ft)

        if groups is None:
            self.__is_running = False
            dia.debug_end('Skipping file')
            return
        if not order:
            order = groups.keys()

        for hl.key in order:
            dia.debug_start()
            hl.group = self._exists(hl.key, '.group', None)
            fgroup = self._exists(hl.key, '.filter.group', None)

            if hl.group is not None and hl.key in groups:
                hl.allow_keyword = self._exists(hl.key, '.allow_keyword', 1)
                hl.prefix = self._exists(hl.key, '.prefix', self.__prefix)
                hl.suffix = self._exists(hl.key, '.suffix', self.__suffix)
                hl.notin = self._exists(hl.key, '.notin', [])

                if not self._highlight(hl, groups[hl.key], force):
                    break
            else:
                dia.error("Unexpected error")

            fkey = hl.key + '_filter'
            if fgroup is not None and fkey in groups:
                fhl = deepcopy(hl)
                fhl.key = fkey
                fhl.allow_keyword = self._exists(hl.key, '.allow_keyword', 1)
                fhl.prefix = self._exists(
                    hl.key, '.filter.prefix', self.__prefix)
                fhl.suffix = self._exists(
                    hl.key, '.filter.suffix', self.__suffix)
                fhl.notin = self._exists(hl.key, '.filter.notin', [])

                if not self._highlight(fhl, groups[fhl.key], force):
                    break

        for group in self.__backup_groups[hl.ft]:
            self._restore_group(hl.ft, group)

        self.__hlbuf = self.__cur['buf'].number
        dia.debug_end('applied syntax for %s' % hl.ft)

##############################################################################
# Projects

    def setBase(self, args):
        do_set_base(dia, self.vv('settings_file'), args)

    def removeBase(self, args):
        do_remove_base(dia, self.vv('settings_file'), args)

##############################################################################
# Private

    def _highlight(self, hl, group, force):
        highlights, number = hl.highlights, hl.number

        dia.debug_echo("Highlighting for buffer %s" % number)
        if number in self.__md5_cache:
            highlights = self.__md5_cache[number]
        else:
            self.__md5_cache[number] = highlights = {}

        current = []
        cmds = []
        hl.key = '_Neotags_%s_%s' % (hl.key.replace('#', '_'), hl.group)
        md5 = hashlib.md5()
        strgrp = b''.join(group)

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
                dia.error('Key error in _highlight()!')
                dia.debug_end('')
                return True
            dia.debug_echo("Updating from cache" % cmds)

        else:
            cmds.append('silent! syntax clear %s' % hl.key)

            for i in range(0, len(group), self.__patternlength):
                current = group[i:i + self.__patternlength]
                current = [x.decode('ascii') for x in current]
                if hl.notin:
                    hl.notin = [*hl.notin, *self.vv('global_notin')]
                    dia.debug_echo(hl.notin)
                    cmds.append(self.__notin_pattern %
                                (hl.key, hl.prefix, r'\|'.join(current),
                                 hl.suffix, ','.join(hl.notin)))
                elif (hl.prefix == self.__prefix and
                      hl.suffix == self.__suffix and
                      hl.allow_keyword == 1 and
                      '.' not in r''.join(current)):
                    cmds.append(self.__keyword_pattern %
                                (hl.key, r' '.join(current)))
                else:
                    cmds.append(self.__match_pattern_not %
                                (hl.key, hl.prefix, r'\|'.join(current), hl.suffix, ','.join(self.vv('global_notin'))))

            if hl.ft != self.vim.api.eval('&ft'):
                dia.debug_end('filetype changed aborting highlight')
                return False

            self.__md5_cache[number][hl.key] = md5hash
            cmds.append('hi def link %s %s' % (hl.key, hl.group))

        full_cmd = ' | '.join(cmds)
        dia.debug_echo(full_cmd)

        if self.__cur['buf'] == self.vim.current.buffer:
            self.vim.command(full_cmd, async_=True)
            success = True
        else:
            dia.debug_echo('Buffer changed, aborting.')
            success = False

        try:
            self.__cmd_cache[number][hl.key] = cmds
        except KeyError:
            self.__cmd_cache[number] = {}
            self.__cmd_cache[number][hl.key] = cmds
        finally:
            if success:
                dia.debug_end('Updated highlight for %s' % hl.key)
            else:
                dia.pop()
            return True

    def _parseTags(self, ft):
        self._get_file()
        files = []

        dia.debug_start()
        dia.debug_echo("Using tags file %s" % self.__gzfile)
        dia.debug_echo("run_ctags -> %d" % self.vv('run_ctags'))

        if not os.path.exists(self.__gzfile):
            if self.vv('run_ctags'):
                self._run_ctags(True)
                files.append(self.__gzfile)
            else:
                self.__gzfile = None
        else:
            dia.debug_echo('updating vim-tagfile')
            with self._open(self.__gzfile, 'rb', self.vv('compression_type')) as fp:
                self._update_vim_tagfile(self.__gzfile, fp)
            files.append(self.__gzfile)

        for File in self.__init_tagfiles:
            if os.path.exists(File):
                files.append(File)

        dia.debug_end("Finished updating file list")

        if not files:
            dia.error('No tag files found!')
            return None

        # Slurp the whole content of the current buffer
        self.__slurp = '\n'.join(self.__cur['buf'])

        if self.__neotags_bin is not None:
            try:
                return self._bin_get_tags(files, ft)
            except CBinError as err:
                self.vim.command("echoerr 'C binary failed with status %d: \"%s\"' "
                                 "| echoerr 'Will try python code.'" % err.args, async_=True)
                return self._get_tags(files, ft)
        else:
            return self._get_tags(files, ft)

    def _get_backup(self, ft, group):
        tmp = self.vim.api.eval("execute('syn list %s')" % group)
        tmp = re.sub(r'.*xxx\s*(.*)\s*links to (.*)',
                     r'\1 \2', tmp, flags=re.S)
        tmp = re.sub(r'(?:\s+|\n)', ' ', tmp).split()

        try:
            self.__backup_groups[ft][group] = (tmp[-1], tmp[:-1])
        except IndexError:
            dia.error("Unexpected index error in _get_backup()")
            self.__backup_groups[ft][group] = []

    def _restore_group(self, ft, group):
        cmds = []
        lnk = self.__backup_groups[ft][group][0]
        symbols = self.__backup_groups[ft][group][1]

        cmds.append('silent! syntax clear %s' % group)
        cmds.append('syntax keyword %s %s' % (group, ' '.join(symbols)))
        cmds.append('hi! link %s %s' % (group, lnk))

        full_cmd = ' | '.join(cmds)
        self.vim.command(full_cmd, async_=True)


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
            file_list = '%s%s%s' % (comp_type, SEPCHAR, files[0])
            for File in files[1:]:
                file_list += '%snone%s%s' % (SEPCHAR, SEPCHAR, File)
        else:
            file_list = ''
            for File in files:
                file_list += '%snone%s%s' % (SEPCHAR, SEPCHAR, File)

        stime = time.time()
        dia.debug_echo("=============== Executing C code ===============")

        ignored_tags = self.vv('ignored_tags')
        if ft in ignored_tags and ignored_tags[ft]:
            ignored_tags = SEPCHAR.join(ignored_tags[ft])
        else:
            ignored_tags = ''

        if equivalent is None:
            equiv_str = ''
        else:
            equiv_str = SEPCHAR.join([A + B for A, B in equivalent.items()])

        indata = self.__slurp.encode('ascii', errors='replace')
        File = self.__cur['file']

        dia.debug_echo("Cmd is: %s" % [
            self.__neotags_bin, file_list, lang, vimlang, order,
            str(self.vv('strip_comments')), len(indata),
            ignored_tags, equiv_str, File])

        # Required arguments (in this order):
        #    1) List of tags files the compression type of each file, with
        #       all fields separated by colons (comptype:file:comptype:file)
        #    2) The language of the current buffer in ctags' format
        #    3) The same in vim's format
        #    4) The `order' string
        #    5) Whether to strip out comments (0 or 1)
        #    6) The length in bytes of the current vim buffer
        #    7) The `ignored' tags list (colon separated)
        #    8) The list of groups considered equivalent (colon separated)
        # All numbers must be converted to strings for the subprocess interface.
        proc = subprocess.Popen(
            (
                self.__neotags_bin,
                file_list,
                lang,
                vimlang,
                order,
                str(self.vv('strip_comments')),
                str(len(indata)),
                ignored_tags,
                equiv_str,
                File
            ),
            stdin=subprocess.PIPE,
            stderr=subprocess.PIPE,
            stdout=subprocess.PIPE,
        )
        out, err = proc.communicate(input=indata)
        if sys.platform == 'win32':
            out = out.rstrip().split(b'\n').rstrip(b'\r')
        else:
            out = out.rstrip().split(b'\n')
        err = err.rstrip().decode(errors='replace').split('\n')

        dia.debug_echo("Returned %d items" % (len(out)))
        for line in err:
            if line:
                dia.debug_echo("ERR: %s" % line)
        if proc.returncode:
            raise CBinError(proc.returncode, err[-1])

        for line in out:
            try:
                key, name = line.split(b'\t')
            except ValueError:
                continue
            key = key.decode()
            try:
                groups[key].append(name)
            except KeyError:
                groups[key] = [name]

        dia.debug_echo("Elapsed time for reading file: %fs" %
                       (float(time.time()) - stime), err=True)

        return groups

# =============================================================================
# No C binary

    def _get_tags(self, files, ft):
        filetypes = ft.lower().split('.')
        languages = ft.lower().split('.')
        dia.debug_echo("=============== Executing Python code ===============")
        match_list = []

        try:
            ignored_tags = self.vv('ignored_tags')[ft]
        except KeyError:
            ignored_tags = []
        try:
            order = self.vim.api.eval('neotags#%s#order' % ft)
        except NvimError:
            dia.debug_echo("No order string found.")
            return
        try:
            equivalent = self.vim.api.eval('neotags#%s#equivalent' % ft)
        except NvimError:
            equivalent = None

        stime = time.time()
        groups = {
            "%s#%s" % (ft, kind): set()
            for kind in [chr(i) for i in order.encode('ascii')]
        }

        if filetypes is None:
            dia.debug_echo("No filetypes identified, returning.")
            return groups
        if self.__gzfile is None:
            comp_type = None
        else:
            comp_type = self.vv('compression_type')

        for File in files:
            try:
                with self._open(File, 'rb', comp_type) as fp:
                    data = fp.read()
                    tags = find_tags(dia, data, self._vim_to_ctags(languages)[0],
                                     order, ignored_tags, equivalent)
                    match_list.append(tags)
            except FileNotFoundError:
                if File == self.__gzfile:
                    dia.error("No tags file found. Make sure Universal Ctags is "
                              "installed and in your $PATH.")
                continue

            comp_type = None

        match_list = [i for s in match_list for i in s]
        self._parse(ft, match_list, groups, languages,
                    ignored_tags, equivalent, order)
        for grp in groups.keys():
            groups[grp] = list(set(groups[grp]))

        dia.debug_echo("Elapsed time for reading file: %fs" %
                       (float(time.time()) - stime), err=True)
        dia.debug_echo("Finished finding tags, found %d items."
                       % sum(map(len, groups.values())))

        return groups

    def _parse(self, ft, match_list, groups, languages, ignored_tags, equivalent, order):
        dia.debug_start()
        key_lang = languages[0]

        if key_lang in ('c', 'cpp', 'java', 'go', 'rust', 'cs'):
            buf = strip_c(self.__slurp, dia)
        else:
            buf = bytes(self.__slurp, 'ascii', errors='replace')

        toks = sorted(tokenize(buf, dia))

        for match in match_list:
            if (bindex(toks, match['name']) != (-1)
                    or b'$' in match['name']
                    or b'.' in match['name']):
                key = "%s#%s" % (ft, match['kind'].decode('ascii'))
                groups[key].add(match['name'])

        dia.debug_end("Finished _parse, found %d items."
                      % sum(map(len, groups.values())))

# =============================================================================

    def _run_ctags(self, force):
        dia.debug_start()
        ctags_command = self._get_ctags_command(force)
        if ctags_command is None:
            dia.debug_end("Not running ctags.")
            return

        try:
            proc = subprocess.Popen(
                ctags_command, shell=True, stderr=subprocess.PIPE)
            proc.wait(self.vv('ctags_timeout'))
            err = proc.communicate()[1]

            if err:
                dia.error('Ctags completed with errors')
                for e in err.decode('ascii').split('\n'):
                    dia.error(e)
            else:
                dia.debug_echo('Ctags completed successfully')

            cmpt = self.vv('compression_type')
            try:
                dia.debug_start()
                if cmpt in ('gzip', 'lzma'):
                    with open(self.__tagfile, 'rb') as src:
                        with self._open(self.__gzfile, 'wb', cmpt, level=9) as dst:
                            dst.write(src.read())

                        src.seek(0)
                        self._update_vim_tagfile(self.__gzfile, src)

                    os.unlink(self.__tagfile)

            except IOError as err:
                dia.error("Unexpected IO Error -> '%s'" % err)

            finally:
                dia.debug_end('Finished compressing file.')

        except FileNotFoundError as error:
            dia.error('failed to run Ctags %s' % error)

        except subprocess.TimeoutExpired:
            try:
                self._kill(proc.pid)
            except ImportError:
                proc.kill()
            else:
                if self.vv('silent_timeout') == 0:
                    self.vim.command("echom 'Ctags process timed out!'",
                                     async_=True)

        finally:
            dia.debug_end("Finished running ctags")

    def _get_ctags_command(self, force):
        """Create the commandline to be invoked when running ctags."""
        ctags_args = self.vv('ctags_args')

        # NOTE: _get_file() sets self.__tagfile and self.__gzfile!
        recurse, path, run = self._get_file()
        if not run or (not force and os.path.exists(self.__gzfile)
                       and os.stat(self.__gzfile).st_size > 0):
            return None

        ctags_args.append('-f "%s"' % self.__tagfile)
        ctags_binary = None

        if recurse:
            if self.vv('find_tool'):
                find_tool = "%s %s" % (self.vv('find_tool'), path)
                if (self.__tagfiles_by_type == 1):
                    ft = self.vim.api.eval('&ft')
                    languages = self._vim_to_ext(ft.lower().split('.'))
                    find_tool = '%s | %s "\\.(%s)$"' % (
                        find_tool, self.vv('regex_tool'), '|'.join(languages))

                ctags_args.append('-L -')
                ctags_binary = "%s | %s" % (
                    find_tool,
                    self.vv('ctags_bin'))
                dia.debug_echo(
                    "Using %s to find files recursively in dir '%s'" %
                    (self.vv('find_tool'), path))
            else:
                ctags_args.append('-R')
                ctags_args.append('"%s"' % path)
                ctags_binary = self.vv('ctags_bin')

                dia.debug_echo("Running ctags on dir '%s'" % path)

        else:
            dia.debug_echo(
                "Not running ctags recursively for dir '%s'" % path)
            File = self.__cur['file']
            ctags_args.append('"%s"' % File)
            ctags_binary = self.vv('ctags_bin')

            dia.debug_echo("Running ctags on file '%s'" % File)

        full_command = '%s %s' % (ctags_binary, ' '.join(ctags_args))
        dia.debug_echo(full_command)

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
        number = self.__cur['buf'].number

        if number in self.__md5_cache.keys():
            highlights = self.__md5_cache[number]
        else:
            self.__md5_cache[number] = highlights = {}

        return highlights, number

    def _clear(self, ft):
        if ft is None:
            dia.debug_echo('Clear called with null ft')
            return

        cmds = []
        order = self._tags_order(ft)

        for key in order:
            hlgroup = self._exists(key, '.group', None)
            hlkey = '_Neotags_%s_%s' % (key.replace('#', '_'), hlgroup)
            cmds.append('silent! syntax clear %s' % hlkey)

        dia.debug_echo(str(cmds))

        self.vim.command(' | '.join(cmds), async_=True)

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

    def _vim_to_ext(self, languages):
        for i, lang in enumerate(languages):
            if lang in self.__vtoe:
                for ext in self.__vtoe[lang]:
                    languages[i] = ext

        return languages

    def _vim_to_ctags(self, languages):
        for i, lang in enumerate(languages):
            if lang in self.__vtoc:
                languages[i] = self.__vtoc[lang]
                languages[i] = languages[i].strip('\\')

            languages[i] = re.escape(languages[i])

        return languages

    def _get_file(self):
        File = self.__cur['file']
        path = os.path.dirname(File)
        projects = {}
        recurse = (self.vv('recursive')
                   and path not in self.vv('norecurse_dirs'))

        if recurse:
            try:
                try:
                    with open(self.vv('settings_file'), 'r') as fp:
                        projects = {
                            p: int(run) for p, run in
                            [j.split('\t') for j in [i.rstrip() for i in fp]]
                        }

                except FileNotFoundError:
                    with open(self.vv('settings_file'), 'x') as fp:
                        fp.write('')

            except ValueError:
                projects = {}  # Just reset projects
                with open(self.vv('settings_file'), 'r') as fp:
                    for line in fp:
                        line = line.rstrip()
                        if line.find('\t') == (-1):
                            projects[line] = 1
                        else:
                            path, run = line.split('\t')
                            projects[path] = int(run)
                with open(self.vv('settings_file'), 'w') as fp:
                    for item in projects:
                        fp.write("%s\t%d\n" % (item, projects[item]))

            run = 1
            for proj_path in projects:
                if os.path.commonpath([path, proj_path]) == proj_path:
                    path = proj_path
                    run = projects[path]
                    break

            path = os.path.realpath(path)
            self._path_replace(path)

        else:
            run = 1
            self._path_replace(File)

        self.vim.command('let g:neotags_file = "%s"' %
                         self.__tagfile, async_=True)

        return recurse, path, run

    def _path_replace(self, path):
        if (sys.platform == 'win32'):
            # For some reason replace wouldn't work here. I have no idea why.
            path = re.sub(':', '__', path)
            sep_char = '\\'
        else:
            sep_char = '/'

        if (self.__tagfiles_by_type == 1):
            ft = self.vim.api.eval('&ft')
            self.__tagfile = "%s/%s_%s.tags" % (self.vv('directory'),
                                                path.replace(sep_char, '__'),
                                                ft)
        else:
            self.__tagfile = "%s/%s.tags" % (self.vv('directory'),
                                             path.replace(sep_char, '__'))
        self.__gzfile = self.__tagfile + self.__fsuffix

    def _get_binary(self, loud=False):
        binary = self.vv('bin')

        if sys.platform == 'win32' and binary.find('.exe') < 0:
            binary += '.exe'

        if os.path.exists(binary):
            self.vv('use_binary', SET=1)
        else:
            self.vv('use_binary', SET=0)
            binary = None
            if loud:
                dia.inform_echo("Binary '%s' doesn't exist. Cannot enable." %
                                self.__neotags_bin)
            else:
                dia.debug_echo(
                    "Binary '%s' doesn't exist." % self.__neotags_bin)

        return binary

    def _update_vim_tagfile(self, tagfile, open_file):
        try:
            if tagfile not in self.__tmp_cache:
                tmpfile = open(self.vim.call('tempname'), 'wb')
                self.__tmp_cache[tagfile] = {
                    'fp': tmpfile,
                    'name': tmpfile.name
                }
                tmpfile.write(open_file.read())
                self.vim.command('set tags+=%s' % tmpfile.name, async_=True)
            else:
                if sys.platform == 'win32':
                    tmpfile = open(self.__tmp_cache[tagfile]['name'], 'wb')
                else:
                    tmpfile = self.__tmp_cache[tagfile]['fp']
                tmpfile.seek(0)
                tmpfile.truncate(0)
                tmpfile.flush()
                tmpfile.write(open_file.read())

            tmpfile.flush()
            # On windows we must close the file or else it will be impossible
            # to delete it when nvim itself closes.
            if sys.platform == 'win32':
                tmpfile.close()

        except IOError as err:
            dia.error("Unexpected io error: %s" % err)

    def _open(self, filename, mode, comp_type, level=None, **kwargs):
        if comp_type not in ('gzip', 'lzma'):
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
            dia.debug_echo("ERROR: varname %s doesn't exist." % varname)
            raise err

###############################################################################
# Toggling. These are ugly and repeditive.

    def toggle(self):
        """Toggle state of the plugin."""
        if not self.vv('enabled'):
            dia.inform_echo("Re-enabling neotags.")
            self.vv('enabled', SET=1)
            self.update(force=False)
        else:
            dia.inform_echo("Disabling neotags.")
            self.vv('enabled', SET=0)
            self.__seen = []
            self.__md5_cache = {}
            self.__cmd_cache = {}
            self.update(force=False)

    def toggle_C_bin(self):
        if self.__neotags_bin is None:
            self.__neotags_bin = self._get_binary(loud=True)
            if self.__neotags_bin is not None:
                dia.inform_echo("Switching to use C binary.")
                self.vv('use_binary', SET=1)
        else:
            self.__neotags_bin = None
            self.vv('use_binary', SET=0)
            dia.inform_echo("Switching to use python code.")

    def toggle_verbosity(self):
        dia.toggle()


###############################################################################


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
        self.highlights = None
        self.number = None


class CBinError(Exception):
    """Dummy wrapper."""
