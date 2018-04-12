if !exists('g:neotags#cpp#order')
	let g:neotags#cpp#order = 'cgstuedfpm'
endif

let g:neotags#cpp#c = {
            \   'group': 'cppTypeTag',
            \   'ignore': '(__anon[0-9a-f]+|[_\w]+::)'
            \ }

let g:neotags#c#m = {
            \   'group': 'cppmembertag',
            \   'ignore': '(__anon[0-9a-f]+|[_\w]+::)',
            \   'prefix': '\%(\%(\>\|\]\|)\)\%(\.\|->\)\)\@5<=',
            \ }

let g:neotags#cpp#g = g:neotags#cpp#c
let g:neotags#cpp#s = g:neotags#cpp#c
let g:neotags#cpp#t = g:neotags#cpp#c
let g:neotags#cpp#u = g:neotags#cpp#c

let g:neotags#cpp#e = {
            \   'group': 'cppEnumTag',
            \   'ignore': '^[_\w]+::'
            \ }

let g:neotags#cpp#d = {
            \   'group': 'cppPreProcTag'
            \ }

let g:neotags#cpp#f = {
            \   'group': 'cppFunctionTag',
            \   'ignore': '^(~|[_\w]+::|operator)',
            \   'suffix': '\>\%(\s*(\)\@='
            \ }

let g:neotags#cpp#p = g:neotags#cpp#f


highlight def link cppEnumTag Define
highlight def link cppFunctionTag Function
highlight def link cppMemberTag Identifier
highlight def link cppPreProcTag PreProc
highlight def link cppTypeTag Type
