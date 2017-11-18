if !exists('g:neotags#sh#order')
	let g:neotags#sh#order = 'f'
endif

let g:neotags#sh#f = {
            \   'group': 'shFunctionTag',
            \   'suffix': '\(\w\|\s*()|()\)\@!'
            \ }

highlight def link shFunctionTag Function
