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

if !exists('g:neotags_directory')
    let g:neotags_directory = expand('~/.vim_tags')
endif
if !isdirectory(g:neotags_directory)
    call mkdir(g:neotags_directory)
endif

if !exists('g:neotags_settings_file')
    let g:neotags_settings_file = expand('~/.vim_tags/neotags.txt')
endif

if !exists('g:neotags_file')
    " let g:neotags_file = './tags'
    let g:neotags_file = ''
endif

if !exists('g:neotags_norecurse_dirs')
    let g:neotags_norecurse_dirs = [
                \ $HOME,
                \ '/',
                \ '/include',
                \ '/usr/include',
                \ '/usr/share',
                \ '/usr/local/include',
                \ '/usr/local/share'
                \ ]
endif

if !exists('g:neotags_ignored_tags')
    let g:neotags_ignored_tags = []
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

if !exists('g:neotags_events_rehighlight')
    let g:neotags_events_rehighlight = [
                \   'FileType',
                \   'Syntax'
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
                \ '--sort=yes',
                \ '--extras=+q'
                \ ]

    if g:neotags_no_autoconf == 1
        call extend(g:neotags_ctags_args, [
                    \ "--exclude='*config.log'",
                    \ "--exclude='*config.guess'",
                    \ "--exclude='*configure'",
                    \ "--exclude='*Makefile.in'",
                    \ "--exclude='*missing'",
                    \ "--exclude='*depcomp'",
                    \ "--exclude='*aclocal.m4'",
                    \ "--exclude='*install-sh'",
                    \ "--exclude='*config.status'",
                    \ "--exclude='*config.h.in'",
                    \ "--exclude='*Makefile'"
                    \])
    endif
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

if !exists('g:neotags_ft_conv')
    let g:neotags_ft_conv = {
                \ 'C++': 'cpp',
                \ 'C#': 'cs',
                \ 'Sh': 'zsh',
                \ }
endif


" }}}

runtime! plugin/neotags/*.vim

let g:loaded_neotags = 1

augroup NeoTags
    autocmd VimEnter * call NeotagsInit()
augroup END

command! NeotagsToggle call NeotagsToggle()
command! -nargs=1 NeotagsAddProject call NeotagsAddProject(<args>)
command! -nargs=1 NeotagsRemoveProject call NeotagsRemoveProject(<args>)

" vim:fdm=marker
