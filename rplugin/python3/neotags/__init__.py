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

    @neovim.autocmd('VimEnter', eval='expand("<afile>")', pattern='*', sync=True)
    def init(self, filename):
        self.__neotags.init()

    @neovim.function('NeotagsHighlight')
    def highlight(self, args):
        self.__neotags.highlight()

    @neovim.function('NeotagsUpdate')
    def update(self, args):
        self.__neotags.update()
