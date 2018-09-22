# ============================================================================
# File:        __init__.py
# Author:      Christian Persson <c0r73x@gmail.com>
# Repository:  https://github.com/c0r73x/neotags.nvim
#              Released under the MIT license
# ============================================================================
import neovim
from neotags.neotags import Neotags


@neovim.plugin
class NeotagsHandlers(object):
    def __init__(self, vim):
        self.__vim = vim
        self.__neotags = None

    @neovim.function('DoNeotagsInit', sync=False)
    def init(self, args):
        self.__neotags = Neotags(self.__vim)
        self.__vim.async_call(self.__neotags.start)

    @neovim.function('DoNeotagsUpdate')
    def highlight(self, args):
        if self.__neotags is not None:
            self.__vim.async_call(self.__neotags.update, False)

    @neovim.function('DoNeotagsForceUpdate')
    def force_update(self, args):
        if self.__neotags is not None:
            self.__vim.async_call(self.__neotags.update, True)

    @neovim.function('DoNeotagsToggle')
    def toggle(self, args):
        self.__vim.async_call(self.__neotags.toggle)

    @neovim.function('DoNeotagsAddProject')
    def setbase(self, args):
        self.__vim.async_call(self.__neotags.setBase, args)

    @neovim.function('DoNeotagsRemoveProject')
    def removebase(self, args):
        self.__vim.async_call(self.__neotags.removeBase, args)

    @neovim.function('DoNeotags_Toggle_C_Binary')
    def toggle_C_bin(self, args):
        self.__vim.async_call(self.__neotags.toggle_C_bin)

    @neovim.function('DoNeotags_Toggle_Verbosity')
    def toggle_verbosity(self, args):
        self.__vim.async_call(self.__neotags.toggle_verbosity)
