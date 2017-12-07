" ============================================================================
" File:        neotags.vim
" Author:      Christian Persson <c0r73x@gmail.com>
" Repository:  https://github.com/c0r73x/neotags.nvim
"              Released under the MIT license
" ============================================================================

" Options {{{
if exists('g:loaded_neotags')
    finish
endif

if !exists('g:neotags_file')
    let g:neotags_file = './tags'
endif

if !exists('g:neotags_events_update')
    let g:neotags_events_update = [
                \   'BufWritePost'
                \ ]
endif

if !exists('g:neotags_events_highlight')
    let g:neotags_events_highlight = [
                \   'BufReadPre',
                \   'BufEnter',
                \ ]
endif

if !exists('g:neotags_enabled')
    let g:neotags_enabled = 0
endif

if !exists('g:neotags_verbose')
    let g:neotags_verbose = 0
endif

if !exists('g:neotags_run_ctags')
    let g:neotags_run_ctags = 1
endif

if !exists('g:neotags_highlight')
    let g:neotags_highlight = 1
endif

if !exists('g:neotags_recursive')
    let g:neotags_recursive = 1
endif

if !exists('g:neotags_appendpath')
    let g:neotags_appendpath = 1
endif

if !exists('g:neotags_ctags_bin')
    let g:neotags_ctags_bin = 'ctags'
endif

if !exists('g:neotags_ctags_timeout')
    let g:neotags_ctags_timeout = 3
endif

if !exists('g:neotags_silent_timeout')
    let g:neotags_silent_timeout = 0
endif

if !exists('g:neotags_patternlength')
    let g:neotags_patternlength = 2048
endif

if !exists('g:neotags_ctags_args')
    let g:neotags_ctags_args = [
                \ '--fields=+l',
                \ '--c-kinds=+p',
                \ '--c++-kinds=+p',
                \ '--sort=no',
                \ '--extras=+q',
                \ ]
endif

if !exists('g:neotags_global_notin')
    let g:neotags_global_notin = [
                \ '.*String.*',
                \ '.*Comment.*',
                \ 'cIncluded',
                \ 'cCppOut2',
                \ 'cCppInElse2',
                \ 'cCppOutIf2',
                \ 'pythonDocTest',
                \ 'pythonDocTest2',
                \ 'NERDTree.*',
                \ ]
endif

if !exists('g:neotags_ignore')
    let g:neotags_ignore = [
                \ 'text',
                \ 'nofile',
                \ 'mail',
                \ 'qf',
                \ ]
endif

" }}}

runtime! plugin/neotags/*.vim

let g:loaded_neotags = 1

augroup NeoTags
    autocmd VimEnter * call NeotagsInit()
augroup END

command! NeotagsToggle call NeotagsToggle()

" vim:fdm=marker
