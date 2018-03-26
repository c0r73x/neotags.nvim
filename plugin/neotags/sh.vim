if !exists('g:neotags#sh#order')
	let g:neotags#sh#order = 'fa'
endif

let g:neotags#sh#f = {
            \   'group': 'shFunctionTag',
            \   'suffix': '\(\w\|\s*()|()\)\@!'
            \ }

let g:neotags#sh#a = {
            \   'group': 'shAliasTag',
            \ }

highlight def link shFunctionTag Function
highlight def link shAliasTag PreProc
