#!/usr/bin/env pypy3
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
from sys import platform


class Neotags(object):

    def __init__(self, vim):
        self.__vim = vim
        self.__prefix = '\C\<'
        self.__suffix = '\>'
        self.__is_running = False
        self.__initialized = False
        self.__start_time = []
        self.__current_file = ''
        self.__groups = {}
        self.__notin = []
        self.__ignore = []

        self.__bufnum = 0
        self.__seen = []
        self.__ignored_tags = []
        self.__directory = None
        self.__noRecurseDirs = None
        self.__settingsFile = None
        self.__slurp = None
        self.__tagfile = None

    def __void(self, *args):
        return

    def init(self):
        if (self.__initialized):
            return

        if (not self.__vim.vars['neotags_verbose']):
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

        # self.__match_pattern = r'syntax match %s /%s\%%(%s\)%s/ containedin=ALLBUT,%s'
        self.__match_pattern = r'syntax match %s /%s\%%(%s\)%s/'
        # self.__keyword_pattern = r'syntax keyword %s %s containedin=ALLBUT,%s'
        self.__keyword_pattern = r'syntax keyword %s %s'
        self.__exists_buffer = {}
        self.__regex_buffer = {}

        self.__directory = self.__vim.vars['neotags_directory']
        self.__settingsFile = self.__vim.vars['neotags_settings_file']
        self.__noRecurseDirs = self.__vim.vars['neotags_norecurse_dirs']
        self.__ignored_tags = self.__vim.vars['neotags_ignored_tags']

        if (self.__vim.vars['neotags_enabled']):
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

            if (self.__vim.vars['loaded_neotags']):
                self.highlight(False)

        # self.update()

        self.__initialized = True

    def toggle(self):
        """Toggle state of the plugin."""
        if (not self.__vim.vars['neotags_enabled']):
            self.__vim.vars['neotags_enabled'] = 1
        else:
            self.__vim.vars['neotags_enabled'] = 0

        self.update()

    def update(self):
        """Update tags file, tags cache, and highlighting."""
        if (not self.__vim.vars['neotags_enabled']):
            self._clear()
            return

        ft = self.__vim.api.eval('&ft')
        if (ft == '' or ft in self.__ignore):
            return

        if (self.__is_running):
            return
        self.__is_running = True

        if (self.__vim.vars['neotags_run_ctags']):
            self._run_ctags()
        self.__groups[ft] = self._parseTags(ft)

        self.__is_running = False
        self.highlight(False)

    def highlight(self, clear):
        """Analyze the tags data and format it for nvim's regex engine."""
        self.__exists_buffer = {}

        if (clear):
            self._clear()
        if (not self.__vim.vars['neotags_enabled']):
            self._clear()
            return
        if (not self.__vim.vars['neotags_highlight']):
            self._clear()
            return

        ft = self.__vim.api.eval('&ft')
        if (ft == '' or ft in self.__ignore):
            return

        if (self.__is_running):
            return
        self.__is_running = True

        self._debug_start()
        file = self.__vim.api.eval("expand('%:p:p')")

        if (ft not in self.__groups) \
                or (self.__bufnum != len(self.__vim.buffers)) \
                or (file not in self.__seen):
            self._debug_echo("Forcing an update!")
            self.__seen.append(file)
            self.__groups[ft] = self._parseTags(ft)

        order = self._tags_order(ft)
        groups = self.__groups[ft]

        if not order:
            order = groups.keys()

        for key in order:
            hlgroup = self._exists(key, '.group', None)
            fgroup = self._exists(key, '.filter.group', None)

            if hlgroup is not None and key in groups:
                prefix = self._exists(key, '.prefix', self.__prefix)
                suffix = self._exists(key, '.suffix', self.__suffix)
                notin = self._exists(key, '.notin', [])

                if not self._highlight(key, file, ft, hlgroup, groups[key],
                                       prefix, suffix, notin):
                    break

                self._debug_echo('applied syntax for %s' % key)

            fkey = key + '_filter'
            if fgroup is not None and fkey in groups:
                prefix = self._exists(key, '.filter.prefix', self.__prefix)
                suffix = self._exists(key, '.filter.suffix', self.__suffix)
                notin = self._exists(key, '.filter.notin', [])

                if not self._highlight(fkey, file, ft, fgroup, groups[fkey],
                                       prefix, suffix, notin):
                    break

                self._debug_echo('applied syntax for %s' % fkey)

        self._debug_end('applied syntax')

        self.__current_file = file
        self.__is_running = False

##############################################################################
    # Projects

    def setBase(self, args):
        try:
            with open(self.__settingsFile, 'r') as fp:
                projects = [i.rstrip for i in fp]
        except FileNotFoundError:
            projects = []

        with open(self.__settingsFile, 'a') as fp:
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
            with open(self.__settingsFile, 'r') as fp:
                projects = [i.rstrip() for i in fp]
        except FileNotFoundError:
            return
        path = os.path.realpath(args[0])

        if path in projects:
            projects.remove(path)
            self._inform_echo(
                "Removed directory '%s' from project list." % path
            )
            with open(self.__settingsFile, 'w') as fp:
                for path in projects:
                    fp.write(path + '\n')
        else:
            self._inform_echo("Error: directory '%s' is not a known project"
                              " base." % path)

##############################################################################
    # Private

    def _parseTags(self, ft):
        self._get_file()
        neotags_file = self.__tagfile
        tagfiles = self.__vim.api.eval('&tags').split(",")

        self._debug_start()
        self._debug_echo("Using tags file " + neotags_file)
        if neotags_file not in tagfiles:
            self.__vim.command('set tags+=%s' % neotags_file, async=True)
            tagfiles.append(neotags_file)

        if not os.path.exists(neotags_file):
            self._debug_echo("Tags file does not exist. Running ctags.")
            self._run_ctags()

        self._debug_end("Finished updating file list")

        files = []

        for f in tagfiles:
            f = f.replace(';', '').encode('utf-8')

            if (os.path.isfile(f)):
                try:
                    if (os.stat(f).st_size > 0):
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
        self._debug_start()

        recurse, path = self._get_file()
        if recurse:
            ctags_args.append('-R')
        else:
            self._debug_echo(
                "Not running ctags recursively for dir '%s'" % path
            )

        ctags_args.append('-f "%s"' % self.__tagfile)

        if recurse:
            ctags_args.append('"%s/"' % path)
            self._debug_echo("Running ctags on dir '%s'" % path)
        else:
            File = os.path.realpath(self.__vim.api.eval("expand('%:p')"))
            ctags_args.append('"%s"' % File)
            self._debug_echo("Running ctags on file '%s'" % File)

        self._debug_echo('%s %s' % (self.__vim.vars['neotags_ctags_bin'],
                         ' '.join(ctags_args)))

        try:
            proc = subprocess.Popen(
                '%s %s' % (self.__vim.vars['neotags_ctags_bin'],
                           ' '.join(ctags_args)),
                shell=True,
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

    def _exists(self, kind, var, default):
        buffer = kind + var

        if buffer not in self.__exists_buffer:
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
            cmds.append('silent! syntax clear %s' % key,)

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

        md5hash = md5.hexdigest()

        if hlkey in highlights and md5hash == highlights[hlkey]:
            self._debug_end('No need to update %s for %s' % (hlkey, file))
            return True
        else:
            cmds.append('silent! syntax clear %s' % hlkey)

        for i in range(0, len(group), self.__patternlength):
            current = group[i:i + self.__patternlength]

            if prefix == self.__prefix and suffix == self.__suffix:
                self._debug_echo("%s is a keyword arg" % current)
                cmds.append(self.__keyword_pattern %
                            (hlkey, ' '.join(current)))
            else:
                cmds.append(self.__match_pattern %
                            (hlkey, prefix, '\|'.join(current), suffix))

            # if prefix == self.__prefix and suffix == self.__suffix:
            #     self._debug_echo("%s is a keyword arg" % current)
            #     cmds.append(self.__keyword_pattern % (
            #         hlkey,
            #         ' '.join(current),
            #         ','.join(self.__notin + notin)
            #     ))
            # else:
            #     cmds.append(self.__match_pattern % (
            #         hlkey,
            #         prefix,
            #         '\|'.join(current),
            #         suffix,
            #         ','.join(self.__notin + notin)
            #     ))

        if ft != self.__vim.api.eval('&ft'):
            self._debug_end('filetype changed aborting highlight')
            return False

        self._debug_echo("Sending command %s" % cmds)
        self._debug_end('Updated highlight for %s' % hlkey)

        highlights[hlkey] = md5hash

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
            self._debug_echo("Ignoring %s based on pattern %s" %
                             (entry['name'], ignore))
            return

        fgroup = self._regexp(kind, '.filter.pattern')

        name = self.__to_escape.sub(r'\\\g<0>', entry['name'])
        if fgroup is not None and fgroup.search(name):
            name = fgroup.sub('', name)
            kind = entry['lang'] + '#' + entry['kind'] + '_filter'

        if kind in groups:
            if name not in groups[kind]:
                if self._check_tags(name):
                    groups[kind].append(name)
        else:
            if self._check_tags(name):
                groups[kind] = [name]

    def _getTags(self, files, ft):
        # Slurp the whole content of the current buffer
        self.__slurp = ''
        self.__bufnum = 0
        self._debug_start()
        for buf in self.__vim.buffers:
            self._debug_echo(str(buf))
            self.__slurp += ''.join(buf)
            self.__bufnum += 1
        self._debug_end("Finished updating slurp")

        filetypes = ft.lower().split('.')
        languages = ft.lower().split('.')
        groups = {}

        if filetypes is None:
            return groups

        lang = '|'.join(self._vim_to_ctags(filetypes))
        pattern = re.compile(
            b'(?:^|\n)(?P<name>[^\t]+)\t(?P<file>[^\t]+)\t\/(?P<cmd>.+)\/;"\t(?P<kind>\w)\tlanguage:(?P<lang>'
            + bytes(lang, 'utf8') + b'(?:\w+)?)', re.IGNORECASE
        )

        for File in files:
            self._debug_start()
            if (os.stat(File).st_size == 0):
                continue
            try:
                with open(File, 'r') as f:
                    mf = mmap.mmap(f.fileno(), 0,  access=mmap.ACCESS_READ)

                    for match in pattern.finditer(mf):
                        self._parseLine(match, groups, languages)

                    mf.close()

            except IOError as e:
                self._error("could not read %s: %s" % (File, e))
                continue

            self._debug_end('done reading %s' % File)

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
            'echom "%s (%.2fs)"' %
            (self.__to_escape.sub(r'\\\g<0>', message).replace('"', r'\"'),
             elapsed)
        )

    def _debug_end(self, message):
        self._debug_echo(message)
        self.__start_time.pop()

    def _inform_echo(self, message):
        self.__vim.command(
            'echom "%s"' %
            self.__to_escape.sub(r'\\\g<0>', message).replace('"', r'\"')
        )

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

    def _check_tags(self, tag):
        """Reject tags that do not appear in the current buffer."""
        # This trick can give an astronomical performance increase when working
        # on a very large project with thousands of tags (tested example was
        # EFL - performance went from unusable to buttery smooth).
        return (self.__slurp.find(tag) > 0) and tag not in self.__ignored_tags

    def _get_file(self):
        File = os.path.realpath(self.__vim.api.eval("expand('%:p')"))
        path = os.path.dirname(File)
        projects = []

        self._debug_start()

        if self.__vim.vars['neotags_recursive']:
            recurse = (path not in self.__noRecurseDirs)

        if recurse:
            try:
                with open(self.__settingsFile, 'r') as fp:
                    projects = [i.rstrip() for i in fp]
            except FileNotFoundError:
                with open(self.__settingsFile, 'x') as fp:
                    fp.write('')

            self._debug_echo(str(projects))
            for proj_path in projects:
                self._debug_echo("Processing '%s'" % proj_path)
                if os.path.commonpath([path, proj_path]) == proj_path:
                    path = proj_path
                    break

            path = os.path.realpath(path)
            self._path_replace(path)

        else:
            self._path_replace(File)

        self.__vim.command('let g:neotags_file = "%s"'
                           % self.__tagfile, async=True)
        self._debug_end("File is '%s'" % self.__tagfile)

        return recurse, path

    def _path_replace(self, path):
        if (platform == 'win32'):
            # For some reason replace wouldn't work here. I have no idea why.
            path = re.sub(':', '__', path)
            sep_char = '\\'
        else:
            sep_char = '/'

        self.__tagfile = "%s/%s.tags" % (self.__directory,
                                         path.replace(sep_char, '__'))
