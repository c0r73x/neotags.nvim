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
        self.__neotags = Neotags(self.__vim)

    @neovim.function('NeotagsInit')
    def init(self, args):
        self.__vim.async_call(self.__neotags.init)

    @neovim.function('NeotagsHighlight')
    def highlight(self, args):
        self.__vim.async_call(self.__neotags.update, False)

    @neovim.function('NeotagsRehighlight')
    def rehighlight(self, args):
        # self.__vim.async_call(self.__neotags.highlight, True)
        self.__vim.async_call(self.__neotags.update, True)

    @neovim.function('NeotagsUpdate')
    def update(self, args):
        self.__vim.async_call(self.__neotags.update, True)

    @neovim.function('NeotagsToggle')
    def toggle(self, args):
        self.__vim.async_call(self.__neotags.toggle)

    @neovim.function('NeotagsAddProject')
    def setbase(self, args):
        self.__vim.async_call(self.__neotags.setBase, args)

    @neovim.function('NeotagsRemoveProject')
    def removebase(self, args):
        self.__vim.async_call(self.__neotags.removeBase, args)

    @neovim.function('Neotags_Toggle_C_Binary')
    def toggle_C_bin(self, args):
        self.__vim.async_call(self.__neotags.toggle_C_bin)

    @neovim.function('Neotags_Toggle_Verbosity')
    def toggle_verbosity(self, args):
        self.__vim.async_call(self.__neotags.toggle_verbosity)
