if !exists('g:neotags#sh#order')
	let g:neotags#sh#order = 'fa'
endif

let g:neotags#sh#f = { 'group': 'FunctionTag' }
let g:neotags#sh#a = { 'group': 'AliasTag' }

highlight def link FunctionTag	neotags_FunctionTag
highlight def link AliasTag	neotags_PreProcTag
