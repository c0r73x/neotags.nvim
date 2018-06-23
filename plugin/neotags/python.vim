if !exists('g:neotags#python#order')
	let g:neotags#python#order = 'mfcv'
endif

let g:neotags#python#m = {
            \   'prefix': '\(\.\|\<def\s\+\)\@<=',
            \   'group': 'pythonMethodTag'
            \ }
let g:neotags#python#f = {
            \   'prefix': '\%(\<def\s\+\)\@<!\<',
            \   'group': 'pythonFunctionTag'
            \ }
let g:neotags#python#c = { 'group': 'pythonClassTag' }
let g:neotags#python#v = { 'group': 'pythonGlobalVarTag' }

highlight def link pythonMethodTag	neotags_MethodTag
highlight def link pythonFunctionTag	neotags_FunctionTag
highlight def link pythonClassTag	neotags_ClassTag
highlight def link pythonGlobalVarTag	neotags_GlobalVarTag
