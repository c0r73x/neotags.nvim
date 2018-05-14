if !exists('g:neotags#php#order')
	let g:neotags#php#order = 'fc'
endif

let g:neotags#php#c = { 'group': 'ClassTag' }
let g:neotags#php#f = {
            \   'group': 'FunctionsTag',
            \   'suffix': '(\@='
            \ }

highlight def link ClassTag	neotags_ClassTag
highlight def link FunctionsTag	neotags_FunctionTag
