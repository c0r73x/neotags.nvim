if !exists('g:neotags#java#order')
	let g:neotags#java#order = 'cim'
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

highlight def link javaClassTag Type
highlight def link javaMethodTag Function
highlight def link javaInterfaceTag Identifier
