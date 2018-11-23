# ============================================================================
# File:        __init__.py
# Author:      Christian Persson <c0r73x@gmail.com>
# Repository:  https://github.com/c0r73x/neotags.nvim
#              Released under the MIT license
# ============================================================================
import pynvim
from neotags.neotags import Neotags


@pynvim.plugin
class NeotagsHandlers(object):

    def __init__(self, vim):
        self.__vim = vim
        self.__neotags = Neotags(self.__vim)

    @pynvim.function('NeotagsInit')
    def init(self, args):
        self.__vim.async_call(self.__neotags.init)

    @pynvim.function('NeotagsHighlight')
    def highlight(self, args):
        self.__vim.async_call(self.__neotags.update, False)

    @pynvim.function('NeotagsRehighlight')
    def rehighlight(self, args):
        # self.__vim.async_call(self.__neotags.highlight, True)
        self.__vim.async_call(self.__neotags.update, True)

    @pynvim.function('NeotagsUpdate')
    def update(self, args):
        self.__vim.async_call(self.__neotags.update, True)

    @pynvim.function('NeotagsToggle')
    def toggle(self, args):
        self.__vim.async_call(self.__neotags.toggle)

    @pynvim.function('NeotagsAddProject')
    def setbase(self, args):
        self.__vim.async_call(self.__neotags.setBase, args)

    @pynvim.function('NeotagsRemoveProject')
    def removebase(self, args):
        self.__vim.async_call(self.__neotags.removeBase, args)

    @pynvim.function('Neotags_Toggle_C_Binary')
    def toggle_C_bin(self, args):
        self.__vim.async_call(self.__neotags.toggle_C_bin)

    @pynvim.function('Neotags_Toggle_Verbosity')
    def toggle_verbosity(self, args):
        self.__vim.async_call(self.__neotags.toggle_verbosity)
