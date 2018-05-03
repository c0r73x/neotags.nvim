if !exists('g:neotags#ruby#order')
	let g:neotags#ruby#order = 'mfc'
endif

let g:neotags#ruby#m = {
            \   'group': 'rubyModuleNameTag',
            \ }

let g:neotags#ruby#c = {
            \   'group': 'rubyClassNameTag',
            \ }

let g:neotags#ruby#f = {
            \   'group': 'rubyMethodNameTag',
            \ }

let g:neotags#ruby#F = g:neotags#ruby#f

highlight def link rubyModuleNameTag Type
highlight def link rubyClassNameTag Type
highlight def link rubyMethodNameTag Function
