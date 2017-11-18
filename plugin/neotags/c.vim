if !exists('g:neotags#c#order')
	let g:neotags#c#order = 'cgstuedfpm'
endif

let g:neotags#c#c = {
            \   'group': 'cTypeTag',
            \   'ignore': '(__anon[0-9a-f]+|[_\w]+::)'
            \ }

let g:neotags#c#m = {
            \   'group': 'cMemberTag',
            \   'ignore': '(__anon[0-9a-f]+|[_\w]+::)'
            \ }

let g:neotags#c#g = g:neotags#c#c
let g:neotags#c#s = g:neotags#c#c
let g:neotags#c#t = g:neotags#c#c
let g:neotags#c#u = g:neotags#c#c

let g:neotags#c#e = {
            \   'group': 'cEnumTag',
            \   'ignore': '^[_\w]+::'
            \ }

let g:neotags#c#d = {
            \   'group': 'cPreProcTag'
            \ }

let g:neotags#c#f = {
            \   'group': 'cFunctionTag',
            \   'ignore': '^(~|[_\w]+::)'
            \ }

let g:neotags#c#p = g:neotags#c#f

highlight def link cEnumTag Define
highlight def link cFunctionTag Function
highlight def link cMemberTag Identifier
highlight def link cPreProcTag PreProc
highlight def link cTypeTag Type
