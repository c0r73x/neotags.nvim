# ============================================================================
# File:        neotags.py
# Author:      Christian Persson <c0r73x@gmail.com>
# Repository:  https://github.com/c0r73x/neotags.nvim
#              Released under the MIT license
# ============================================================================
import os
import re
import mmap
import time
import hashlib
import subprocess

import psutil


class Neotags(object):

    def __init__(self, vim):
        self.__vim = vim
        self.__prefix = '\C\<'
        self.__suffix = '\>'
        self.__is_running = False
        self.__initialized = False
        self.__start_time = []
        self.__current_file = ''
        self.__current_type = None
        self.__groups = {}
        self.__notin = []
        self.__ignore = []

    def __void(self, *args):
        return

    def init(self):
        if(self.__initialized):
            return

        if(not self.__vim.vars['neotags_verbose']):
            self._debug_start = self.__void
            self._debug_echo = self.__void
            self._debug_end = self.__void

        self.__notin = self.__vim.vars['neotags_global_notin']
        self.__ignore = self.__vim.vars['neotags_ignore']
        self.__current_file = self.__vim.api.eval("expand('%:p:p')")
        self.__to_escape = re.compile(r'[.*^$/\\~\[\]]')

        self.__ctov = self.__vim.vars['neotags_ft_conv']
        self.__vtoc = {
            y: x for x,
            y in self.__ctov.items()
        }

        self.__pattern = r'syntax match %s /%s\%%(%s\)%s/ containedin=ALLBUT,%s'
        self.__exists_buffer = {}
        self.__regex_buffer = {}

        if(self.__vim.vars['neotags_enabled']):
            evupd = ','.join(self.__vim.vars['neotags_events_update'])
            evhl = ','.join(self.__vim.vars['neotags_events_highlight'])
            evre = ','.join(self.__vim.vars['neotags_events_rehighlight'])

            self.__patternlength = self.__vim.vars['neotags_patternlength']

            self.__vim.command(
                'autocmd %s * call NeotagsUpdate()' % evupd,
                async=True
            )
            self.__vim.command(
                'autocmd %s * call NeotagsHighlight()' % evhl,
                async=True
            )
            self.__vim.command(
                'autocmd %s * call NeotagsRehighlight()' % evre,
                async=True
            )

            if(self.__vim.vars['loaded_neotags']):
                self.highlight(False)

        self.__initialized = True

    def toggle(self):
        if(not self.__vim.vars['neotags_enabled']):
            self.__vim.vars['neotags_enabled'] = 1
        else:
            self.__vim.vars['neotags_enabled'] = 0

        self.update()

    def update(self):
        if(not self.__vim.vars['neotags_enabled']):
            self._clear()
            return

        ft = self.__vim.api.eval('&ft')

        if ft == '' or ft in self.__ignore:
            return

        if(self.__is_running):
            return

        self.__is_running = True

        if(self.__vim.vars['neotags_run_ctags']):
            self._run_ctags()

        self.__groups[ft] = self._parsetags(ft)

        self.__is_running = False

        self.highlight(False)

    def highlight(self, clear):
        self.__exists_buffer = {}

        if(clear):
            self._clear()

        if(not self.__vim.vars['neotags_enabled']):
            self._clear()
            return

        if(not self.__vim.vars['neotags_highlight']):
            self._clear()
            return

        ft = self.__vim.api.eval('&ft')

        if ft == '' or ft in self.__ignore:
            return

        if(self.__is_running):
            return

        self.__is_running = True

        if ft not in self.__groups:
            self.__groups[ft] = self._parsetags(ft)

        order = self._tags_order(ft)
        groups = self.__groups[ft]

        if not order:
            order = groups.keys()

        file = self.__vim.api.eval("expand('%:p:p')")

        self._debug_start()
        for key in order:

            hlgroup = self._exists(key, '.group', None)
            filter = self._exists(key, '.filter.group', None)

            if hlgroup is not None and key in groups:
                prefix = self._exists(key, '.prefix', self.__prefix)
                suffix = self._exists(key, '.suffix', self.__suffix)
                notin = self._exists(key, '.notin', [])

                if (not self._highlight(
                    key,
                    file,
                    ft,
                    hlgroup,
                    groups[key],
                    prefix,
                    suffix,
                    notin
                )):
                    break

                self._debug_echo('applied syntax for %s' % key)

            fkey = key + '_filter',
            if filter is not None and fkey in groups:
                prefix = self._exists(key, '.filter.prefix', self.__prefix)
                suffix = self._exists(key, '.filter.suffix', self.__suffix)
                notin = self._exists(key, '.filter.notin', [])

                if (not self._highlight(
                    fkey,
                    file,
                    ft,
                    filter,
                    groups[fkey],
                    prefix,
                    suffix,
                    notin
                )):
                    break

                self._debug_echo('applied syntax for %s' % fkey)

        self._debug_end('applied syntax')

        self.__current_file = file
        self.__is_running = False

    def _parsetags(self, ft):
        neotags_file = self.__vim.vars['neotags_file']
        tagfiles = self.__vim.api.eval('&tags').split(",")
        if neotags_file not in tagfiles:
            self.__vim.command('set tags+=%s' % neotags_file, async=True)
            tagfiles.append(neotags_file)

        files = []

        for f in tagfiles:
            f = f.replace(';', '').encode('utf-8')

            if(os.path.isfile(f)):
                try:
                    if(os.stat(f).st_size > 0):
                        files.append(f)
                except IOError:
                    self._error('unable to open %s' % f.decode('utf-8'))

        if files is None:
            self._error("echom 'No tag files found!'")
            return

        return self._getTags(files, ft)

    def _tags_order(self, ft):
        orderlist = []
        filetypes = ft.lower().split('.')

        for filetype in filetypes:
            order = self._exists(filetype, '#order', None)

            if order:
                orderlist += [(filetype + '#') + s for s in list(order)]

        return orderlist

    def _kill(self, proc_pid):
        process = psutil.Process(proc_pid)
        for proc in process.children():
            proc.kill()
        process.kill()

    def _run_ctags(self):
        ctags_args = self.__vim.vars['neotags_ctags_args']

        if(self.__vim.vars['neotags_recursive']):
            ctags_args.append('-R')

        ctags_args.append('-f-')

        if(self.__vim.vars['neotags_appendpath']):
            ctags_args.append('"%s"' % self.__vim.funcs.getcwd())

        file = open(self.__vim.vars['neotags_file'], 'wb')

        self._debug_start()

        try:
            proc = subprocess.Popen('%s %s' % (
                self.__vim.vars['neotags_ctags_bin'], ' '.join(ctags_args)
                ),
                shell=True,
                stdout=file,
                stderr=subprocess.PIPE
            )

            proc.wait(self.__vim.vars['neotags_ctags_timeout'])
            err = proc.communicate()[1]
            if err:
                self._error('Ctags completed with errors')

                for e in err.decode('ascii').split('\n'):
                    self._error(e)
            else:
                self._debug_end('Ctags completed successfully')
        except FileNotFoundError as error:
            self._error('failed to run Ctags %s' % error)
        except subprocess.TimeoutExpired:
            self._kill(proc.pid)

            if self.__vim.vars['neotags_silent_timeout'] == 0:
                self.__vim.command(
                    "echom 'Ctags process timed out!'",
                    async=True
                )

        file.close()

    def _exists(self, kind, var, default):
        buffer = kind + var

        if buffer in self.__exists_buffer:
            return self.__exists_buffer[buffer]

        if self.__vim.funcs.exists("neotags#%s" % buffer):
            self.__exists_buffer[buffer] = self.__vim.api.eval(
                'neotags#%s' % buffer
            )
        else:
            self.__exists_buffer[buffer] = default

        return self.__exists_buffer[buffer]

    def _getbufferhl(self):
        if not self.__vim.funcs.exists('b:neotags_cache'):
            self.__vim.command('let b:neotags_cache = {}')

        return self.__vim.api.eval('b:neotags_cache')

    def _clear(self):
        highlights = self._getbufferhl()

        cmds = []

        for key in highlights.keys():
            cmds.append(
                'silent! syntax clear %s' % key,
            )

        cmds.append('let b:neotags_cache = {}')
        self.__vim.command(' | '.join(cmds), async=True)

    def _highlight(self, key, file, ft, hlgroup, group, prefix, suffix, notin):
        highlights = self._getbufferhl()

        current = []
        cmds = []
        hlkey = '_Neotags_%s_%s' % (key.replace('#', '_'), hlgroup)

        self._debug_start()

        md5 = hashlib.md5()
        strgrp = ''.join(group).encode('utf8')

        for i in range(0, len(strgrp), 128):
            md5.update(strgrp[i:i + 128])

        hash = md5.hexdigest()

        if hlkey in highlights and hash == highlights[hlkey]:
            self._debug_end('No need to update %s for %s' % (hlkey, file))
            return True
        else:
            cmds.append('silent! syntax clear %s' % hlkey)

        for i in range(0, len(group), self.__patternlength):
            current = group[i:i + self.__patternlength]

            cmds.append(self.__pattern % (
                hlkey,
                prefix,
                '\|'.join(current),
                suffix,
                ','.join(self.__notin + notin)
            ))

        if ft != self.__vim.api.eval('&ft'):
            self._debug_end('filetype changed aborting highlight')
            return False

        self._debug_end('Updated highlight for %s' % hlkey)

        highlights[hlkey] = hash

        cmds.append('let b:neotags_cache = %s' % highlights)
        cmds.append('hi link %s %s' % (hlkey, hlgroup))

        self.__vim.command(' | '.join(cmds), async=True)
        return True

    def _regexp(self, kind, var):
        buffer = kind + var

        if buffer in self.__regex_buffer:
            return self.__regex_buffer[buffer]

        str = self._exists(kind, var, None)

        if str is not None:
            regexp = re.compile(r'%s' % str)
            self.__regex_buffer[buffer] = regexp
            return regexp

        return None

    def _parseLine(self, match, groups, languages):
        entry = {
            x: ''.join(map(chr, y)) for x,
            y in match.groupdict().items()
        }

        entry['lang'] = self._ctags_to_vim(entry['lang'], languages)

        kind = entry['lang'] + '#' + entry['kind']
        ignore = self._regexp(kind, '.ignore')

        if ignore and ignore.search(entry['name']):
            self._debug_echo(
                "Ignoring %s based on pattern %s" % (entry['name'], ignore)
            )
            return

        filter = self._regexp(kind, '.filter.pattern')

        name = self.__to_escape.sub(r'\\\g<0>', entry['name'])
        if filter is not None and filter.search(name):
            kind = entry['lang'] + '#' + entry['kind'] + '_filter'

        if kind in groups:
            if name not in groups[kind]:
                groups[kind].append(name)
        else:
            groups[kind] = [name]

    def _getTags(self, files, ft):
        filetypes = ft.lower().split('.')
        languages = ft.lower().split('.')
        groups = {}

        if filetypes is None:
            return groups

        lang = '|'.join(self._vim_to_ctags(filetypes))
        pattern = re.compile(
            b'(?:^|\n)(?P<name>[^\t]+)\t(?P<file>[^\t]+)\t\/(?P<cmd>.+)\/;"\t(?P<kind>\w)\tlanguage:(?P<lang>' + bytes(lang, 'utf8') + b'(?:\w+)?)',
            re.IGNORECASE
        )

        for file in files:
            self._debug_start()

            if(os.stat(file).st_size == 0):
                continue

            try:
                with open(file, 'r') as f:
                    mf = mmap.mmap(f.fileno(), 0,  access=mmap.ACCESS_READ)

                    for match in pattern.finditer(mf):
                        self._parseLine(match, groups, languages)

                    mf.close()
            except IOError as e:
                self._error("could not read %s: %s" % (file, e))
                continue

            self._debug_end('done reading %s' % file)

        order = self._tags_order(ft)

        if not order:
            order = list(groups.keys())

        clean = reversed(order)

        self._debug_start()

        for a in list(clean):
            if a not in groups:
                continue

            for b in list(order):
                if b not in groups or a == b:
                    continue

                groups[a] = [x for x in groups[a] if x not in groups[b]]

        self._debug_end('done cleaning groups')

        return groups

    def _debug_start(self):
        self.__start_time.append(time.time())

    def _debug_echo(self, message):
        elapsed = time.time() - self.__start_time[-1]
        self.__vim.command(
            'echom "%s (%.2fs)"' % (
                self.__to_escape.sub(r'\\\g<0>', message),
                elapsed
            )
        )

    def _debug_end(self, message):
        self._debug_echo(message)
        self.__start_time.pop()

    def _error(self, message):
        if message:
            message = message.replace('\\', '\\\\').replace('"', '\\"')
            self.__vim.command(
                'echohl ErrorMsg | echom "%s" | echohl None' % message
            )

    def _ctags_to_vim(self, lang, languages):
        if lang in self.__ctov and self.__ctov[lang] in languages:
            return self.__ctov[lang]

        return lang.lower()

    def _vim_to_ctags(self, languages):
        for i, lang in enumerate(languages):

            if lang in self.__vtoc:
                languages[i] = self.__vtoc[lang]

            languages[i] = re.escape(languages[i])

        return languages
