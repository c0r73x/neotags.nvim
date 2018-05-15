if !exists('g:neotags#ruby#order')
	let g:neotags#ruby#order = 'mfc'
endif

let g:neotags#ruby#m = { 'group': 'rubyModuleTag' }
let g:neotags#ruby#c = { 'group': 'rubyClassTag' }
let g:neotags#ruby#f = { 'group': 'rubyMethodTag' }

let g:neotags#ruby#equivalent = { 'F': 'f' }

highlight def link rubyModuleTag	neotags_ModuleTag
highlight def link rubyClassTag	neotags_ClassTag
highlight def link rubyMethodTag	neotags_MethodTag
