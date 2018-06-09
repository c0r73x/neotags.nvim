if !exists('g:neotags#sh#order')
	let g:neotags#sh#order = 'fa'
endif

let g:neotags#sh#f = { 'group': 'shFunctionTag' }
let g:neotags#sh#a = { 'group': 'shAliasTag' }

highlight def link shFunctionTag	neotags_FunctionTag
highlight def link shAliasTag	neotags_PreProcTag
