if !exists('g:neotags#ruby#order')
	let g:neotags#ruby#order = 'mfc'
endif

let g:neotags#ruby#m = { 'group': 'ModuleTag' }
let g:neotags#ruby#c = { 'group': 'ClassTag' }
let g:neotags#ruby#f = { 'group': 'MethodTag' }

let g:neotags#ruby#equivalent = { 'F': 'f' }

highlight def link ModuleTag	neotags_ModuleTag
highlight def link ClassTag	neotags_ClassTag
highlight def link MethodTag	neotags_MethodTag
