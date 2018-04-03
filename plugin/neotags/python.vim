if !exists('g:neotags#python#order')
	let g:neotags#python#order = 'mfc'
endif

let g:neotags#python#m = {
            \   'prefix': '\(\.\|\<def\s\+\)\@<=',
            \   'group': 'pythonMethodTag'
            \ }
let g:neotags#python#f = {
            \   'prefix': '\%(\<def\s\+\)\@<!\<',
            \   'group': 'pythonFunctionTag'
            \ }

let g:neotags#python#c = {
            \   'group': 'pythonClassTag'
            \ }

highlight def link PythonMethodTag Function
highlight def link PythonFunction Function
highlight def link PythonClassTag Type
