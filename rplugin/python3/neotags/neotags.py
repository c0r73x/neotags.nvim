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
from neovim.api.nvim import NvimError

# sys.path.append(os.path.dirname(__file__))
from neotags.utils import (do_set_base, do_remove_base, find_tags, strip_c,
                           tokenize, bindex)
from neotags.diagnostics import Diagnostics

CLIB = None
dia = None
vim = None
settings = None
if sys.platform == 'win32':
    SEPCHAR = ';'
else:
    SEPCHAR = ':'


def vimvar(varname, SET=None):
    """Either return a nvim variable prepended with 'neotags_', or set that
    variable to the value of SET and then return that value.
    """
    try:
        if SET is None:
            return vim.vars["neotags_" + varname]
        else:
            vim.vars["neotags_" + varname] = SET
            return SET

    except (NvimError, KeyError) as err:
        dia.debug_echo("ERROR: varname %s doesn't exist." % varname)
        raise err


def open_gzfile(filename, mode, comp_type, level=None, **kwargs):
    if comp_type not in ('gzip', 'lzma'):
        string = 'open(filename, mode, **kwargs)'

    elif comp_type == 'gzip':
        string = 'CLIB.open(filename, mode, %s **kwargs)' % (
            '' if level is None else 'compresslevel=level,')

    elif comp_type == 'lzma':
        string = 'CLIB.open(filename, mode, %s **kwargs)' % (
            '' if level is None else 'preset=level,')

    return eval(string)


class Neotags(object):
    def __init__(self, nvim_obj):
        self.buflist = {}

        self.__prefix = r'\C\<'
        self.__suffix = r'\>'
        self.__initialized = False
        self.__is_running = False

        self.__cmd_cache = {}
        self.__exists_buffer = {}
        self.__groups = {}
        self.__regex_buffer = {}
        self.__tmp_cache = {}
        self.__cur = {'file': None, 'buf': None}

        self.__seen = []
        self.__backup_groups = {}

        self.__autocmd = None
        self.__init_tagfiles = None
        self.__patternlength = None
        self.__slurp = None

        self.__keyword_pattern = ''
        self.__match_pattern = ''
        self.__match_pattern_not = ''
        self.__notin_pattern = ''
        self.__to_escape = ''

        self.__hlbuf = 1

        global vim
        vim = nvim_obj

    def init(self):
        if self.__initialized:
            return

        global dia, settings
        dia = Diagnostics(bool(vimvar('verbose')), vim, settings)
        settings = Settings()

        self.__init_tagfiles = vim.api.eval('&tags').split(",")
        self.__to_escape = re.compile(r'[.*^$/\\~\[\]]')
        self.__notin_pattern = r'syntax match %s /%s\%%(%s\)%s/ containedin=ALLBUT,%s display'
        self.__match_pattern_not = r'syntax match %s /%s\%%(%s\)%s/ containedin=ALLBUT,%s display'
        self.__match_pattern = r'syntax match %s /%s\%%(%s\)%s/ display'
        self.__keyword_pattern = r'syntax keyword %s %s'

        self.__autocmd = "if execute('autocmd User') =~# 'NeotagsPost' | " \
                         "doautocmd User NeotagsPost | endif"
        self.__initialized = True

        if settings.enabled:
            evupd = ','.join(vimvar('events_update'))
            evhl = ','.join(vimvar('events_highlight'))
            evre = ','.join(vimvar('events_rehighlight'))

            self.__patternlength = vimvar('patternlength')

            vim.command('autocmd %s * call NeotagsUpdate()' % evupd, async_=True)
            vim.command('autocmd %s * call NeotagsHighlight()' % evhl, async_=True)
            vim.command('autocmd %s * call NeotagsRehighlight()' % evre, async_=True)

            if vimvar('loaded'):
                self.update(False)

    def update(self, force):
        """Update tags file, tags cache, and highlighting."""
        buf = vim.current.buffer
        if buf.number in self.buflist:
            bdata = self.buflist[buf.number]
        else:
            bdata = Bufdata(buf, vim.api.eval('&ft'))

        init_time = time.time()

        if not settings.enabled:
            self._clear(bdata.ft)
            vim.command(self.__autocmd, async_=True)
            return
        if bdata.ft == '' or bdata.ft in settings.ignore or vim.api.eval('&previewwindow'):
            vim.command(self.__autocmd, async_=True)
            return
        if self.__is_running:
            # XXX This should be more robust
            return

        dia.debug_start()
        self.__is_running = True
        self.__exists_buffer = {}
        self.__cur['file'] = bdata.name.full
        self.__cur['buf'] = vim.current.buffer
        # hl.highlights = self._getbufferhl(bdata.num)

        self._run_ctags(bdata, force)

        if force:
            if self.__run_ctags:
                self._run_ctags(True)
            self.__groups[bdata.ft] = self._parseTags(bdata)
        elif bdata.num not in self.__seen:
            self.__seen.append(self.__cur['buf'].number)
            self.__groups[bdata.ft] = self._parseTags(bdata)
        elif bdata.num not in self.__cmd_cache:
            self.__groups[bdata.ft] = self._parseTags(bdata)

        self.highlight(force, bdata)

        vim.command(self.__autocmd, async_=True)
        dia.clear_stack()
        dia.debug_echo('Finished all => (%.4fs)' % (time.time() - init_time))
        self.__is_running = False

    def highlight(self, force, bdata):
        """Analyze the tags data and format it for nvim's regex engine."""
        restored_groups = settings.restored_groups
        hl = HighlightGroup(bdata)

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
                hl.prefix = self._exists(hl.key, '.prefix', self.__prefix)
                hl.suffix = self._exists(hl.key, '.suffix', self.__suffix)
                hl.notin = self._exists(hl.key, '.notin', [])

                if not self._highlight(bdata, hl, groups[hl.key], force):
                    break
            else:
                dia.error("Unexpected error -> %s : %s : %s" % (hl.group, hl.key, groups.keys()))

            fkey = hl.key + '_filter'
            if fgroup is not None and fkey in groups:
                fhl = deepcopy(hl)
                fhl.key = fkey
                fhl.prefix = self._exists(hl.key, '.filter.prefix', self.__prefix)
                fhl.suffix = self._exists(hl.key, '.filter.suffix', self.__suffix)
                fhl.notin = self._exists(hl.key, '.filter.notin', [])

                if not self._highlight(bdata, fhl, groups[fhl.key], force):
                    break

        for group in self.__backup_groups[hl.ft]:
            self._restore_group(hl.ft, group)

        self.__hlbuf = self.__cur['buf'].number
        dia.debug_end('applied syntax for %s' % hl.ft)

##############################################################################
# Projects

    def setBase(self, args):
        do_set_base(dia, settings.settings_file, args)

    def removeBase(self, args):
        do_remove_base(dia, settings.settings_file, args)

##############################################################################
# Private

    def _highlight(self, bdata, hl, group, force):
        highlights, number = hl.highlights, hl.number

        dia.debug_echo("Highlighting for buffer %s" % number)
        highlights = bdata.md5_cache

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
                    hl.notin = [*hl.notin, *settings.global_notin]
                    dia.debug_echo(hl.notin)
                    cmds.append(self.__notin_pattern %
                                (hl.key, hl.prefix, r'\|'.join(current),
                                 hl.suffix, ','.join(hl.notin)))
                elif (hl.prefix == self.__prefix and
                      hl.suffix == self.__suffix and
                      '.' not in r''.join(current)):
                    cmds.append(self.__keyword_pattern %
                                (hl.key, r' '.join(current)))
                else:
                    cmds.append(self.__match_pattern_not %
                                (hl.key, hl.prefix, r'\|'.join(current), hl.suffix, ','.join(settings.global_notin)))

            if hl.ft != vim.api.eval('&ft'):
                dia.debug_end('filetype changed aborting highlight')
                return False

            bdata.md5_cache[hl.key] = md5hash
            cmds.append('hi def link %s %s' % (hl.key, hl.group))

        full_cmd = ' | '.join(cmds)
        dia.debug_echo(full_cmd)

        if self.__cur['buf'] == vim.current.buffer:
            vim.command(full_cmd, async_=True)
            success = True
        else:
            dia.error('Buffer changed, aborting.')
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

    def _parseTags(self, bdata):
        files = []

        dia.debug_start()
        dia.debug_echo("Using tags file %s" % bdata.gzfile)
        dia.debug_echo("run_ctags -> %d" % settings.should_run_ctags)

        if not os.path.exists(bdata.gzfile):
            if settings.should_run_ctags:
                self._run_ctags(True)
                files.append(bdata.gzfile)
            else:
                self.__gzfile = None
        else:
            dia.debug_echo('updating vim-tagfile')
            with open_gzfile(bdata.gzfile, 'rb', settings.compression_type) as fp:
                self._update_vim_tagfile(bdata.gzfile, fp)
            files.append(bdata.gzfile)

        for File in self.__init_tagfiles:
            if os.path.exists(File):
                files.append(File)

        dia.debug_end("Finished updating file list")

        if not files:
            dia.error('No tag files found!')
            return None

        # Slurp the whole content of the current buffer
        self.__slurp = '\n'.join(self.__cur['buf'])

        if settings.neotags_bin is not None:
            try:
                return bdata.bin_get_tags(self.__slurp, files)
            except CBinError as err:
                vim.command("echoerr 'C binary failed with status %d: \"%s\"' "
                            "| echoerr 'Will try python code.'" % err.args, async_=True)
                return bdata.get_tags(self.__slurp, files)
        else:
            return bdata.get_tags(self.__slurp, files)

    def _get_backup(self, ft, group):
        tmp = vim.api.eval("execute('syn list %s')" % group)
        tmp = re.sub(r'.*xxx\s*(.*)\s*links to (.*)', r'\1 \2', tmp, flags=re.S)
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
        self._parse(ft, match_list, groups, languages, ignored_tags, equivalent, order)
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
=======
        vim.command(full_cmd, async_=True)
>>>>>>> Slight refactor/reorganization


# =============================================================================

    def _run_ctags(self, bdata, force):
        dia.debug_start()
        ctags_command = bdata.get_ctags_command(force)
        if ctags_command is None:
            dia.debug_end("Not running ctags.")
            return

        try:
            proc = subprocess.Popen(
                ctags_command, shell=True, stderr=subprocess.PIPE)
            proc.wait(settings.ctags_timeout)
            err = proc.communicate()[1]

            if err:
                dia.error('Ctags completed with errors')
                for e in err.decode('ascii').split('\n'):
                    dia.error(e)
            else:
                dia.debug_echo('Ctags completed successfully')

            cmpt = settings.compression_type
            try:
                dia.debug_start()
                if cmpt in ('gzip', 'lzma'):
                    with open(bdata.tagfile, 'rb') as src:
                        with open_gzfile(bdata.gzfile, 'wb', cmpt, level=9) as dst:
                            dst.write(src.read())
                        src.seek(0)
                        self._update_vim_tagfile(bdata.gzfile, src)
                    os.unlink(bdata.tagfile)
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
                if settings.silent_timeout == 0:
                    vim.command("echom 'Ctags process timed out!'", async_=True)
        finally:
            dia.debug_end("Finished running ctags")

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
                self.__exists_buffer[buf] = vim.api.eval('neotags#' + buf)
            except NvimError:
                self.__exists_buffer[buf] = default

        return self.__exists_buffer[buf]

    def _clear(self, ft):
        if ft is None:
            dia.debug_echo('Clear called with null ft')
            return

        highlights, _ = self._getbufferhl()
        cmds = []
        order = self._tags_order(ft)

        for key in order:
            hlgroup = self._exists(key, '.group', None)
            hlkey = '_Neotags_%s_%s' % (key.replace('#', '_'), hlgroup)
            cmds.append('silent! syntax clear %s' % hlkey)

        dia.debug_echo(str(cmds))

        vim.command(' | '.join(cmds), async_=True)

    def _kill(self, proc_pid):
        import psutil

        process = psutil.Process(proc_pid)
        for proc in process.children():
            proc.kill()
        process.kill()

    def _update_vim_tagfile(self, tagfile, open_file):
        try:
            if tagfile not in self.__tmp_cache:
                tmpfile = open(vim.call('tempname'), 'wb')
                self.__tmp_cache[tagfile] = {
                    'fp': tmpfile,
                    'name': tmpfile.name
                }
                tmpfile.write(open_file.read())
                vim.command('set tags+=%s' % tmpfile.name, async_=True)
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

###############################################################################
# Toggling. These are ugly and repeditive.

    def toggle(self):
        """Toggle state of the plugin."""
        self.__hlbuf = 0
        if not settings.enabled:
            dia.inform_echo("Re-enabling neotags.")
            settings.setvar('enabled', 1)
            self.update(force=False)
        else:
            dia.inform_echo("Disabling neotags.")
            settings.setvar('enabled', 0)
            self.__seen = []
            self.__md5_cache = {}
            self.__cmd_cache = {}
            self.update(force=False)

    def toggle_C_bin(self):
        if settings.neotags_bin is None:
            settings.neotags_bin = self._get_binary(loud=True)
            if settings.neotags_bin is not None:
                dia.inform_echo("Switching to use C binary.")
                settings.setvar('use_binary', 1)
        else:
            settings.neotags_bin = None
            settings.setvar('use_binary', 0)
            dia.inform_echo("Switching to use python code.")

    def toggle_verbosity(self):
        dia.toggle()


###############################################################################


class HighlightGroup:
    """Exists to keep the number of arguments being passed around down."""
    def __init__(self, bdata):
        self.file = bdata.name.full
        self.ft = bdata.ft
        self.group = None
        self.key = None
        self.notin = None
        self.prefix = None
        self.suffix = None
        self.highlights = bdata.md5_cache
        self.number = bdata.num


class CBinError(Exception):
    """Dummy wrapper."""
