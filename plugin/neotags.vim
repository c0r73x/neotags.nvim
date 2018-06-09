" ============================================================================
" File:        neotags.vim
" Author:      Christian Persson <c0r73x@gmail.com>
" Repository:  https://github.com/c0r73x/neotags.nvim
"              Released under the MIT license
" ============================================================================

" Options {{{
if exists('g:neotags_loaded')
    finish
endif

function! InitVar(varname, val)
    let l:varname = 'g:neotags_' . a:varname
    if !exists(l:varname)
        execute 'let ' . l:varname . ' = ' . string(a:val)
    endif
endfunction

call InitVar('file', '')
call InitVar('ctags_bin', 'ctags')

call InitVar('ignored_tags', {})
call InitVar('ignored_dirs', [])

call InitVar('directory',     expand('~/.vim_tags'))
call InitVar('bin',           expand(g:neotags_directory . '/bin/neotags'))
call InitVar('settings_file', expand(g:neotags_directory . '/neotags.txt'))

if file_readable(g:neotags_bin)
    call InitVar('use_binary',  1)
else
    call InitVar('use_binary',  0)
endif

call InitVar('use_compression',   1)
call InitVar('compression_level', 9)
call InitVar('compression_type', 'gzip')

call InitVar('enabled',     1)
call InitVar('appendpath',  1)
call InitVar('find_tool',   0)
call InitVar('highlight',   1)
call InitVar('no_autoconf', 1)
call InitVar('recursive',   1)
call InitVar('run_ctags',   1)
call InitVar('verbose',     0)
call InitVar('strip_comments', 1)
call InitVar('silent_timeout', 0)
call InitVar('ctags_timeout',  240)
call InitVar('patternlength',  2048)

" People often make annoying #defines for C and C++ keywords, types, etc. Avoid
" highlighting these by default, leaving the built in vim highlighting intact.
call InitVar('restored_groups', {
                \     'c':   ['cConstant', 'cStorageClass', 'cConditional', 'cRepeat', 'cType'],
                \     'cpp': ['cConstant', 'cStorageClass', 'cConditional', 'cRepeat', 'cType',
                \             'cppStorageClass', 'cppType'],
                \ })
 
call InitVar('norecurse_dirs', [
                \ $HOME,
                \ '/',
                \ '/lib',
                \ '/include',
                \ '/usr/lib/',
                \ '/usr/share',
                \ '/usr/include',
                \ '/usr/local/lib',
                \ '/usr/local/share',
                \ '/usr/local/include',
                \ ])

call InitVar('events_update', [
                \   'BufWritePost'
                \ ])

call InitVar('events_highlight', [
                \   'BufReadPre',
                \   'BufEnter',
                \ ])

call InitVar('events_rehighlight', [
                \   'FileType',
                \   'Syntax',
                \ ])

if g:neotags_run_ctags
    let s:found = 0

    if executable(g:neotags_ctags_bin) && system(g:neotags_ctags_bin . ' --version') =~# 'Universal'
        let s:found = 1
    elseif executable('ctags') && system('ctags --version') =~# 'Universal'
        let g:neotags_ctags_bin = 'ctags'
        let s:found = 1
    elseif executable('uctags') && system('uctags --version') =~# 'Universal'
        let g:neotags_ctags_bin = 'uctags'
        let s:found = 1
    endif

    if s:found
        call InitVar('ctags_args', [
                \   '--fields=+l',
                \   '--c-kinds=+p',
                \   '--c++-kinds=+p',
                \   '--sort=yes',
                \   "--exclude='.mypy_cache'",
                \   '--regex-go=''/^\s*(var)?\s*(\w*)\s*:?=\s*func/\2/f/'''
                \ ])
    else
        echohl ErrorMsg
        echom 'Neotags: Universal Ctags not found, cannot run ctags.'
        echohl None
        let g:neotags_run_ctags = 0
    endif
endif

if g:neotags_run_ctags && g:neotags_no_autoconf == 1
    call extend(g:neotags_ctags_args, [
                \   "--exclude='*Makefile'",
                \   "--exclude='*Makefile.in'",
                \   "--exclude='*aclocal.m4'",
                \   "--exclude='*config.guess'",
                \   "--exclude='*config.h.in'",
                \   "--exclude='*config.log'",
                \   "--exclude='*config.status'",
                \   "--exclude='*configure'",
                \   "--exclude='*depcomp'",
                \   "--exclude='*install-sh'",
                \   "--exclude='*missing'",
                \])
endif

call InitVar('global_notin', [
                \   '.*String.*',
                \   '.*Comment.*',
                \   'cIncluded',
                \   'cCppOut2',
                \   'cCppInElse2',
                \   'cCppOutIf2',
                \   'pythonDocTest',
                \   'pythonDocTest2',
                \ ])

call InitVar('ignore', [
                \   'cfg',
                \   'conf',
                \   'help',
                \   'mail',
                \   'markdown',
                \   'nerdtree',
                \   'nofile',
                \   'qf',
                \   'text',
                \ ])

call InitVar('ft_conv', {
                \   "C++": 'cpp',
                \   'C#': 'cs',
                \   'Sh': 'zsh',
                \ })

if !isdirectory(g:neotags_directory)
    call mkdir(g:neotags_directory)
endif

if !isdirectory(g:neotags_directory . '/bin')
    call mkdir(g:neotags_directory . '/bin')
endif

" }}}

runtime! plugin/neotags/*.vim

let g:neotags_loaded = 1

if v:vim_did_enter
    call NeotagsInit()
else
    augroup NeoTags
        autocmd VimEnter * call NeotagsInit()
    augroup END
endif

function! s:Add_Remove_Project(operation, ctags, ...)
    if exists('a:1')
        let l:path = a:1
    else
        let l:path = getcwd()
    endif
    if a:operation ==# 0
        call NeotagsRemoveProject(a:ctags, l:path)
    elseif a:operation ==# 1
        call NeotagsAddProject(a:ctags, l:path)
    endif
endfunction

" command! -nargs=1 -complete=file NeotagsAddProject call NeotagsAddProject(<f-args>)
" command! -nargs=1 -complete=file NeotagsRemoveProject call NeotagsRemoveProject(<f-args>)
command! -nargs=? -complete=file NeotagsToggleProject call s:Add_remove_Project(2, <q-args>)
command! -nargs=? -complete=file NeotagsAddProject call s:Add_Remove_Project(1, <q-args>, 1)
command! -nargs=? -complete=file NeotagsAddProjectNoCtags call s:Add_Remove_Project(1, <q-args>, 0)
command! -nargs=? -complete=file NeotagsRemoveProject call s:Add_Remove_Project(0, <q-args>, 0)
command! NeotagsToggle call NeotagsToggle()
command! NeotagsVerbosity call Neotags_Toggle_Verbosity()
command! NeotagsBinaryToggle call Neotags_Toggle_C_Binary()

nnoremap <unique> <Plug>NeotagsToggle :call NeotagsToggle()<CR>
nmap <silent> <leader>tag <Plug>NeotagsToggle


"============================================================================= 


highlight def link neotags_ClassTag		neotags_TypeTag
highlight def link neotags_EnumTypeTag		neotags_TypeTag
highlight def link neotags_StructTag		neotags_TypeTag
highlight def link neotags_UnionTag		neotags_TypeTag
highlight def link neotags_MethodTag		neotags_FunctionTag
highlight def link neotags_VariableTag		neotags_ObjectTag
highlight def link neotags_FieldTag		neotags_MemberTag

highlight def link neotags_ConstantTag		Constant
highlight def link neotags_EnumTag		Define
highlight def link neotags_FunctionTag		Function
highlight def link neotags_InterfaceTag		Identifier
highlight def link neotags_MemberTag		Identifier
highlight def link neotags_ObjectTag		Identifier
highlight def link neotags_ModuleTag		PreProc
highlight def link neotags_PreProcTag		PreProc
highlight def link neotags_TypeTag		Type


" vim:fdm=marker
