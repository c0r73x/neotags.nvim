import inspect
import re
import time


class Diagnostics:
    def __init__(self, enabled, vim, vv):
        self.vim = vim
        self.vv = vv
        self.__backup = [self.debug_echo, self.debug_start, self.debug_end]
        self.__start_time = []
        self.__to_escape = re.compile(r'[.*^$/\\~\[\]]')

        if not enabled:
            self.toggle()

    def __void(self, *_, **__):
        return

    def debug_start(self):
        self.__start_time.append(time.time())

    def debug_echo(self, message, err=False):
        if err:
            self.error(message)
        else:
            self.inform_echo(message)

    def debug_end(self, message):
        otime = self.__start_time.pop()
        try:
            self.debug_echo('%d: (%.4fs) END => %s' % (inspect.stack()[1][2],
                            time.time() - otime, message))
        except IndexError:
            self.inform_echo(message)

    def inform_echo(self, message):
        self.vim.command('echom "%s"' % self.__to_escape.sub(
                         r'\\\g<0>', message).replace('"', r'\"'))

    def error(self, message):
        if message:
            message = 'Neotags: ' + message
            message = message.replace('\\', '\\\\').replace('"', '\\"')
            self.vim.command('echohl ErrorMsg | echom "%s" | echohl None' %
                             message)

    def clear_stack(self):
        while self.__start_time:
            self.debug_end("Value:")
            self.error("Extra value in self.__start_time...")

    def toggle(self):
        if self.debug_echo == self.__void:
            self.inform_echo('Switching to verbose output.')
            (self.debug_echo, self.debug_start, self.debug_end) = self.__backup
            self.vv('verbose', SET=1)
        else:
            self.inform_echo('Switching off verbose output.')
            self.debug_start = self.debug_echo = self.debug_end = self.__void
            self.vv('verbose', SET=0)
