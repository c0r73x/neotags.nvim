" ============================================================================
" File:        neotags.py
" Author:      Christian Persson <c0r73x@gmail.com>
" Repository:  https://github.com/c0r73x/neotags.nvim
"              Released under the MIT license
" ============================================================================

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
                \   'BufReadPost'
                \ ]
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

if !exists('g:neotags_ctags_bin')
    let g:neotags_ctags_bin = 'ctags'
endif

if !exists('g:neotags_ctags_args')
    let g:neotags_ctags_args = [
                \ '--fields=+l',
                \ '--c-kinds=+p',
                \ '--c++-kinds=+p',
                \ '--sort=no',
                \ '--extra=+q'
                \ ]
endif

" C++ {{{1
let g:neotags#cpp#c = {
            \   'group': 'cppType'
            \ }

let g:neotags#cpp#g = g:neotags#cpp#c
let g:neotags#cpp#s = g:neotags#cpp#c
let g:neotags#cpp#t = g:neotags#cpp#c
let g:neotags#cpp#u = g:neotags#cpp#c

let g:neotags#cpp#e = {
            \   'group': 'cppEnum'
            \ }

let g:neotags#cpp#d = {
            \   'group': 'cppPreProc'
            \ }

let g:neotags#cpp#f = {
            \   'group': 'cppFunction'
            \ }

let g:neotags#cpp#p = g:neotags#cpp#f
" 1}}}
" C++ {{{1
let g:neotags#c#c = {
            \   'group': 'cType'
            \ }

let g:neotags#c#g = g:neotags#cpp#c
let g:neotags#c#s = g:neotags#cpp#c
let g:neotags#c#t = g:neotags#cpp#c
let g:neotags#c#u = g:neotags#cpp#c

let g:neotags#c#e = {
            \   'group': 'cEnum'
            \ }

let g:neotags#c#d = {
            \   'group': 'cPreProc'
            \ }

let g:neotags#c#f = {
            \   'group': 'cFunction'
            \ }

let g:neotags#c#p = g:neotags#cpp#f
" 1}}}
" Python {{{1
let g:neotags#python#m = {
            \   'prefix': '\(\.\|\<def\s\+\)\@<=',
            \   'group': 'pythonMethod'
            \ }
let g:neotags#python#f = {
            \   'prefix': '\%(\<def\s\+\)\@<!\<',
            \   'group': 'pythonFunction'
            \ }

let g:neotags#python#c = {
            \   'group': 'pythonClass'
            \ }
" 1}}}
" Ruby {{{1
let g:neotags#ruby#m = {
            \   'group': 'rubyModuleName',
            \ }

let g:neotags#ruby#c = {
            \   'group': 'rubyClassName',
            \ }

let g:neotags#ruby#f = {
            \   'group': 'rubyMethodName',
            \ }

let g:neotags#ruby#F = g:neotags#ruby#f
" 1}}}
" Shell {{{1
let g:neotags#sh#f = {
            \   'group': 'shFunction',
            \   'suffix': '\(\w\|\s*()|()\)\@!'
            \ }
" 1}}}
" Java {{{1
let g:neotags#java#c = {
            \   'group': 'javaClass'
            \ }

let g:neotags#java#i = {
            \   'group': 'javaInterface'
            \ }

let g:neotags#java#m = {
            \   'group': 'javaMethod'
            \ }
" 1}}}
" JavaScript {{{1
let g:neotags#javascript#f = {
            \   'group': 'javascriptFunction'
            \ }
" 1}}}

highlight def link rubyModuleName Type
highlight def link rubyClassName Type
highlight def link rubyMethodName Function

highlight def link PythonMethodTag pythonFunction
highlight def link PythonClassTag pythonFunction

highlight def link cEnum Identifier
highlight def link cFunction Function

highlight def link cppEnum Identifier
highlight def link cppFunction Function

highlight def link shFunctionTag Operator
highlight def link perlFunctionTag Operator

highlight def link javaClass Identifier
highlight def link javaMethod Function
highlight def link javaInterface Identifier

highlight def link javascriptFunctionTag Identifier

let g:loaded_neotags = 1
