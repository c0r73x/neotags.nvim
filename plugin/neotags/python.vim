if !exists('g:neotags#python#order')
	let g:neotags#python#order = 'mfc'
endif

let g:neotags#python#m = {
            \   'prefix': '\(\.\|\<def\s\+\)\@<=',
            \   'group': 'MethodTag'
            \ }
let g:neotags#python#f = {
            \   'prefix': '\%(\<def\s\+\)\@<!\<',
            \   'group': 'FunctionTag'
            \ }
let g:neotags#python#c = { 'group': 'ClassTag' }

highlight def link MethodTag	neotags_MethodTag
highlight def link FunctionTag	neotags_FunctionTag
highlight def link ClassTag	neotags_ClassTag
