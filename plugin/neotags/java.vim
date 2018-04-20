if !exists('g:neotags#java#order')
	let g:neotags#java#order = 'cimegf'
endif

let g:neotags#java#c = {
            \   'group': 'javaClassTag'
            \ }

let g:neotags#java#i = {
            \   'group': 'javaInterfaceTag'
            \ }

let g:neotags#java#m = {
            \   'group': 'javaMethodTag'
            \ }

let g:neotags#java#e = {
            \   'group': 'javaEnumTag'
            \ }

let g:neotags#java#g = {
            \   'group': 'javaEnumTypeTag'
            \ }

let g:neotags#java#f = {
            \   'group': 'javaFieldTag'
            \ }

highlight def link javaClassTag Type
highlight def link javaMethodTag Function
highlight def link javaInterfaceTag Identifier
highlight def link javaEnumTag Define
highlight def link javaEnumTypeTag Define
highlight def link javaFieldTag Identifier
