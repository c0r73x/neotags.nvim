if !exists('g:neotags#php#order')
	let g:neotags#php#order = 'fc'
endif

let g:neotags#php#c = { 'group': 'phpClassTag' }
let g:neotags#php#f = {
            \   'group': 'phpFunctionsTag',
            \   'suffix': '(\@='
            \ }

highlight def link phpClassTag	neotags_ClassTag
highlight def link phpFunctionsTag	neotags_FunctionTag
