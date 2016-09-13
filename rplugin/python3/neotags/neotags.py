# ============================================================================
# File:        neotags.py
# Author:      Christian Persson <c0r73x@gmail.com>
# Repository:  https://github.com/c0r73x/neotags.nvim
#              Released under the MIT license
# ============================================================================
import os
import re

from ctags import CTags, TagEntry


class Neotags(object):

    def __init__(self, vim):
        self.__vim = vim
        self.__prefix = '\C\<'
        self.__suffix = '\>'
        self.__is_running = False

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

    def init(self):
        self.__pattern = r'syntax match %s /%s\%%(%s\)%s/ containedin=ALLBUT,%s'
        self.__ctags = self.__vim.eval('g:neotags_ctags_bin')
        self.__output = self.__vim.eval('g:neotags_file')
        self.__vim.command('set tags+=%s' % self.__output)

        self.__pwd = self.__vim.eval('getcwd()')

        self.__ctags_args = self.__vim.eval('g:neotags_ctags_args')

        if(self.__vim.eval('g:neotags_recursive')):
            self.__ctags_args.append('-R')

        self.__ctags_args.append('-f-')
        self.__ctags_args.append('"%s"' % self.__pwd)

        if(self.__vim.eval('g:neotags_enabled')):
            self.highlight()

            evupd = ','.join(self.__vim.eval('g:neotags_events_update'))
            evhl = ','.join(self.__vim.eval('g:neotags_events_highlight'))

            self.__vim.command('autocmd %s * call NeotagsUpdate()' % evupd)
            self.__vim.command('autocmd %s * call NeotagsHighlight()' % evhl)

    def update(self):
        if(not self.__vim.eval('g:neotags_enabled')):
            return

        if(self.__vim.eval('g:neotags_run_ctags')):
            self._run_ctags()

        self.highlight()

    def highlight(self):
        if(not self.__vim.eval('g:neotags_enabled')):
            return

        if(self.__is_running or not self.__vim.eval('g:neotags_highlight')):
            return

        self.__is_running = True

        files = []

        for f in self.__vim.eval('&tags').split(","):
            f = re.sub(r';', '', f).encode('utf-8')

            if(os.path.isfile(f)):
                try:
                    files.append(CTags(f))
                except:
                    self.__vim.command(
                        'echom "Error: unable to open %s"' % f.decode('utf-8')
                    )

        if files is None:
            self.__vim.command("echom 'No tag files found!'")
            return

        groups = self._getTags(files)

        for key, group in groups.items():
            prefix = self._exists(key, 'prefix', self.__prefix)
            suffix = self._exists(key, 'suffix', self.__suffix)
            notin = self._exists(key, 'notin', [])
            hlgroup = self._exists(key, 'group', None)

            if hlgroup is not None:
                self._highlight(hlgroup, group, prefix, suffix, notin)

        self.__is_running = False

    def _run_ctags(self):
        os.system('%s %s > "%s"' % (
            self.__ctags,
            ' '.join(self.__ctags_args),
            self.__output
        ))

    def _exists(self, kind, var, default):
        r = self.__vim.eval('exists("g:neotags#%s.%s")' % (kind, var))
        if r == 1:
            return self.__vim.eval('g:neotags#%s.%s' % (kind, var))
        else:
            return default

    def _highlight(self, key, group, prefix, suffix, notin):
        current = []

        self.__vim.command('silent! syntax clear %s' % key)

        for i in range(0, len(group), 2048):
            current = group[i:i + 2048]

            self.__vim.command(self.__pattern % (
                key + 'Tag',
                prefix,
                '\|'.join(current),
                suffix,
                ','.join(self.__ignore + notin)
            ), async=True)

    def _getTags(self, files):
        filetype = self.__vim.eval('&ft').lower()
        groups = {}

        to_escape = re.compile(r'[.*^$/\\~\[\]]')

        for t in files:
            entry = TagEntry()
            status = t.first(entry)

            while status:
                lang = self._ctags_to_vim(entry['language'.encode('utf-8')])

                if lang == filetype:
                    kind = entry['kind'.encode('utf-8')]
                    key = lang + "#" + kind.decode('utf-8')

                    if key not in groups:
                        groups[key] = []

                    name = to_escape.sub(
                        r'\\\g<0>',
                        entry['name'].decode('utf-8')
                    )

                    if name not in groups[key]:
                        groups[key].append(name)

                status = t.next(entry)

        return groups

    def _ctags_to_vim(self, lang):
        if lang is None:
            return 'unknown'

        lang = lang.decode('utf-8')

        if lang == 'C++':
            return 'cpp'
        if lang == 'C#':
            return 'cs'
        else:
            return lang.lower()
