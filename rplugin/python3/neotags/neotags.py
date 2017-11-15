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
        self.__highlights = {}
        self.__start_time = []
        self.__current_file = ''

        self.__ignore = [
            '.*String.*',
            '.*Comment.*',
            'cIncluded',
            'cCppOut2',
            'cCppInElse2',
            'cCppOutIf2',
            'pythonDocTest',
            'pythonDocTest2'
        ]

    def __void(self, *args):
        return

    def init(self):
        if(self.__initialized):
            return

        if(not self.__vim.vars['neotags_verbose']):
            self._debug_start = self.__void
            self._debug_echo = self.__void
            self._debug_end = self.__void

        self.__current_file = self.__vim.eval("expand('%:p:p')")
        self.__pattern = r'syntax match %s /%s\%%(%s\)%s/ containedin=ALLBUT,%s'
        self.__exists_buffer = {}

        if(self.__vim.vars['neotags_enabled']):
            evupd = ','.join(self.__vim.vars['neotags_events_update'])
            evhl = ','.join(self.__vim.vars['neotags_events_highlight'])

            self.__patternlength = self.__vim.vars['neotags_patternlength']

            self.__vim.command('autocmd %s * call NeotagsUpdate()' % evupd)
            self.__vim.command('autocmd %s * call NeotagsHighlight()' % evhl)

            if(self.__vim.vars['loaded_neotags']):
                self.highlight()

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

        if(self.__vim.vars['neotags_run_ctags']):
            self._run_ctags()

        self.highlight()

    def highlight(self):
        self.__exists_buffer = {}

        if(not self.__vim.vars['neotags_enabled']):
            self._clear()
            return

        if(not self.__vim.vars['neotags_highlight']):
            self._clear()
            return

        neotags_file = self.__vim.vars['neotags_file']
        tagfiles = self.__vim.eval('&tags').split(",")
        if neotags_file not in tagfiles:
            self.__vim.command('set tags+=%s' % neotags_file)
            tagfiles.append(neotags_file)

        files = []

        for f in tagfiles:
            f = re.sub(r';', '', f).encode('utf-8')

            if(os.path.isfile(f)):
                try:
                    if(os.stat(f).st_size > 0):
                        files.append(f)
                except IOError:
                    self._error('unable to open %s' % f.decode('utf-8'))

        if files is None:
            self.__vim.command("echom 'No tag files found!'")
            return

        groups, kinds = self._getTags(files)
        order = self._tags_order()

        if not order:
            order = kinds

        cmds = []

        file = self.__vim.eval("expand('%:p:p')")

        for key in order:
            self._debug_start()

            hlgroup = self._exists(key, '.group', None)
            filter = self._exists(key, '.filter.group', None)

            if hlgroup is not None and hlgroup in groups:
                prefix = self._exists(key, '.prefix', self.__prefix)
                suffix = self._exists(key, '.suffix', self.__suffix)
                notin = self._exists(key, '.notin', [])

                cmds += self._highlight(
                    file,
                    hlgroup,
                    groups[hlgroup],
                    prefix,
                    suffix,
                    notin
                )

            if filter is not None and filter in groups:
                prefix = self._exists(key, '.filter.prefix', self.__prefix)
                suffix = self._exists(key, '.filter.suffix', self.__suffix)
                notin = self._exists(key, '.filter.notin', [])

                cmds += self._highlight(
                    file,
                    filter,
                    groups[filter],
                    prefix,
                    suffix,
                    notin
                )

            self._debug_end('applied syntax for %s' % key)

        self.__current_file = file
        [self.__vim.command(cmd) for cmd in cmds]

    def _tags_order(self):
        orderlist = []
        filetypes = self.__vim.eval('&ft').lower().split('.')

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
        if(self.__is_running):
            return

        self.__is_running = True

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
        except subprocess.FileNotFoundError as error:
            self._error('failed to run Ctags %s' % error)
        except subprocess.TimeoutExpired:
            self._kill(proc.pid)

            if self.__vim.vars['neotags_silent_timeout'] == 0:
                self.__vim.command("echom 'Ctags process timed out!'")

        file.close()

        self.__is_running = False

    def _exists(self, kind, var, default):
        buffer = kind + var

        if buffer in self.__exists_buffer:
            return self.__exists_buffer[buffer]

        if self.__vim.funcs.exists("neotags#%s" % buffer):
            self.__exists_buffer[buffer] = self.__vim.eval(
                'neotags#%s' % buffer
            )
        else:
            self.__exists_buffer[buffer] = default

        return self.__exists_buffer[buffer]

    def _clear(self):
        for key in self.__highlights.keys():
            self.__vim.command(
                'silent! syntax clear %s' % key,
                async=True
            )

        self.__highlights = {}

    def _highlight(self, file, key, group, prefix, suffix, notin):
        current = []
        cmd = []

        self._debug_start()

        hash = hashlib.md5(''.join(group).encode('utf-8')).hexdigest()

        if self.__current_file == file:
            if key in self.__highlights and hash == self.__highlights[key]:
                self._debug_end('No need to update %s for %s' % (key, file))
                return []

        cmd.append('silent! syntax clear %s' % key)

        for i in range(0, len(group), self.__patternlength):
            current = group[i:i + self.__patternlength]

            cmd.append(self.__pattern % (
                key,
                prefix,
                '\|'.join(current),
                suffix,
                ','.join(self.__ignore + notin)
            ))

        self._debug_end('Updated highlight for %s' % key)
        self.__highlights[key] = hash

        return cmd

    def _parseLine(self, match, groups, kinds, to_escape):
        entry = {
            'name': str(match[1], 'utf8'),
            'file': str(match[2], 'utf8'),
            'cmd': str(match[3], 'utf8', errors='ignore'),
            'kind': str(match[4], 'utf8'),
            'lang': self._ctags_to_vim(str(match[5], 'utf8'))
        }

        kind = entry['lang'] + '#' + entry['kind']
        ignore = self._exists(kind, '.ignore', None)

        if ignore and re.search(ignore, entry['name']):
            self._debug_echo(
                "Ignoring %s based on pattern %s" % (entry['name'], ignore)
            )
            return

        if kind not in kinds:
            kinds.append(kind)

        hlgroup = self._exists(kind, '.group', None)
        fstr = self._exists(kind, '.filter.pattern', None)
        filter = None

        if fstr is not None:
            filter = re.compile(r"%s" % fstr)

        if hlgroup is not None:
            cmd = entry['cmd']
            name = to_escape.sub(r'\\\g<0>', entry['name'])

            if filter is not None and filter.search(cmd):
                fgrp = self._exists(kind, '.filter.group', None)

                if fgrp is not None:
                    hlgroup = fgrp

            try:
                groups[hlgroup].append(name)
            except KeyError:
                groups[hlgroup] = [name]

    def _getTags(self, files):
        filetypes = self.__vim.eval('&ft').lower().split('.')
        groups = {}
        kinds = []

        if filetypes is None:
            return groups, kinds

        to_escape = re.compile(r'[.*^$/\\~\[\]]')

        lang = '|'.join(self._vim_to_ctags(filetypes))
        pattern = re.compile(
            b'(^|\n)([^\t]+)\t([^\t]+)\t\/(.+)\/;"\t(\w)\tlanguage:(' + bytes(lang, 'utf8') + b')[\t\n]',
            re.IGNORECASE
        )

        for file in files:
            self._debug_start()

            if(os.stat(file).st_size == 0):
                continue

            try:
                with open(file, 'rb') as f:
                    mf = mmap.mmap(f.fileno(), 0,  access=mmap.ACCESS_READ)
                    for match in pattern.findall(mf):
                        self._parseLine(
                            match,
                            groups,
                            kinds,
                            to_escape
                        )

                    mf.close()
            except IOError as e:
                self._error("could not read %s: %s" % (file, e))
                continue

            self._debug_end('done reading %s' % file)

        order = self._tags_order()

        if not order:
            order = kinds

        clean = [self._exists(a, '.group', None) for a in reversed(order)]

        self._debug_start()

        for a in clean:
            if a not in groups:
                continue

            for b in clean:
                if b not in groups or a == b:
                    continue

                groups[a] = [x for x in groups[a] if x not in groups[b]]

        self._debug_end('done cleaning groups')

        return groups, kinds

    def _debug_start(self):
        self.__start_time.append(time.time())

    def _debug_echo(self, message):
        to_escape = re.compile(r'[.*^$/\\~\[\]]')
        elapsed = time.time() - self.__start_time[-1]
        self.__vim.command(
            'echom "%s (%.2fs)"' % (
                to_escape.sub(r'\\\g<0>', message),
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

    def _ctags_to_vim(self, lang):
        if lang is None:
            return 'unknown'

        if lang == 'C++':
            return 'cpp'
        elif lang == 'C#':
            return 'cs'

        return lang.lower()

    def _vim_to_ctags(self, languages):
        for i, l in enumerate(languages):
            if languages[i] == 'cpp':
                languages[i] = 'C++'
            elif languages[i] == 'cs':
                languages[i] = 'C#'

            languages[i] = re.escape(languages[i])

        return languages
