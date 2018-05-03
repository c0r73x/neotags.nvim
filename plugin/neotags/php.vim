if !exists('g:neotags#php#order')
	let g:neotags#php#order = 'fc'
endif

let g:neotags#php#f = {
            \   'group': 'phpFunctionsTag',
            \   'suffix': '(\@='
            \ }

let g:neotags#php#c = {
            \   'group': 'phpClassesTag'
            \ }

highlight def link phpClassesTag Type
highlight def link phpFunctionsTag Function
