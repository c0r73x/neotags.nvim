# ============================================================================
# File:        neotags.py
# Author:      Christian Persson <c0r73x@gmail.com>
# Repository:  https://github.com/c0r73x/neotags.nvim
#              Released under the MIT license
# ============================================================================
import os
import re


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
        if(not self.__vim.eval('exists("g:loaded_neotags")')):
            return

        self.__pattern = r'syntax match %s /%s\%%(%s\)%s/ containedin=ALLBUT,%s'

        self.__ctags = self.__vim.eval('g:neotags_ctags_bin')
        self.__output = self.__vim.eval('g:neotags_file')

        self.__vim.command('set tags+=%s' % self.__output)
        self.__exists_buffer = {}

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

        groups, kinds = self._getTags()
        order = self._tags_order()

        if not order:
            order = kinds

        prevgroups = []

        for key in order:
            hlgroup = self._exists(key, '.group', None)
            filter = self._exists(key, '.filter.group', None)

            if hlgroup is not None and hlgroup in groups:
                prefix = self._exists(key, '.prefix', self.__prefix)
                suffix = self._exists(key, '.suffix', self.__suffix)
                notin = self._exists(key, '.notin', [])

                nohl = [n for n in prevgroups if not n == hlgroup] + notin
                self._highlight(hlgroup, groups[hlgroup], prefix, suffix, nohl)

                if(hlgroup not in prevgroups):
                    prevgroups.append(hlgroup)

            if filter is not None and filter in groups:
                prefix = self._exists(key, '.filter.prefix', self.__prefix)
                suffix = self._exists(key, '.filter.suffix', self.__suffix)
                notin = self._exists(key, '.filter.notin', [])

                nohl = [n for n in prevgroups if not n == filter] + notin
                self._highlight(filter, groups[filter], prefix, suffix, nohl)

                if(filter not in prevgroups):
                    prevgroups.append(filter)

        self.__is_running = False

    def _tags_order(self):
        filetype = self.__vim.eval('&ft').lower()

        if(len(filetype) > 0):
            order = self._exists(filetype, '#order', None)

            if order:
                return [(filetype + '#') + s for s in list(order)]

        return []

    def _run_ctags(self):
        os.system('%s %s > "%s"' % (
            self.__ctags,
            ' '.join(self.__ctags_args),
            self.__output
        ))

    def _exists(self, kind, var, default):
        buffer = kind + var

        if buffer in self.__exists_buffer:
            return self.__exists_buffer[buffer]

        r = self.__vim.eval('exists("g:neotags#%s")' % buffer)
        if r == 1:
            self.__exists_buffer[buffer] = self.__vim.eval(
                'g:neotags#%s' % buffer
            )
        else:
            self.__exists_buffer[buffer] = default

        return self.__exists_buffer[buffer]

    def _highlight(self, key, group, prefix, suffix, notin):
        current = []

        self.__vim.command('silent! syntax clear %s' % key)

        for i in range(0, len(group), 2048):
            current = group[i:i + 2048]

            self.__vim.command(self.__pattern % (
                key,
                prefix,
                '\|'.join(current),
                suffix,
                ','.join(self.__ignore + notin)
            ), async=True)

    def _getTags(self):
        filetype = self.__vim.eval('&ft').lower()
        groups = {}
        kinds = []

        to_escape = re.compile(r'[.*^$/\\~\[\]]')

        vimtags = self.__vim.eval('taglist(".*")')

        for entry in vimtags:
            lang = self._ctags_to_vim(entry['language'])

            if lang == filetype:
                kind = lang + "#" + entry['kind']

                if kind not in kinds:
                    kinds.append(kind)

                hlgroup = self._exists(kind, '.group', None)
                fstr = self._exists(kind, '.filter.pattern', None)
                filter = None

                if fstr is not None:
                    filter = re.compile(r"%s" % fstr)

                if hlgroup is not None:

                    pattern = entry['cmd']

                    name = to_escape.sub(
                        r'\\\g<0>',
                        entry['name']
                    )

                    if filter is not None and filter.search(pattern):
                        fgrp = self._exists(kind, '.filter.group', None)

                        if fgrp is not None:
                            hlgroup = fgrp

                    if hlgroup not in groups:
                        groups[hlgroup] = []

                    if name not in groups[hlgroup]:
                        groups[hlgroup].append(name)

        return groups, kinds

    def _ctags_to_vim(self, lang):
        if lang is None:
            return 'unknown'

        if lang == 'C++':
            return 'cpp'
        if lang == 'C#':
            return 'cs'
        else:
            return lang.lower()
