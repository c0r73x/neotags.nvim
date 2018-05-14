if !exists('g:neotags#perl#order')
	let g:neotags#perl#order = 's'
endif

let g:neotags#perl#s = {
            \   'group': 'perlFunctionTag',
            \   'prefix': '\%(\<sub\s\*\)\@<!\%(>\|\s\|&\|^\)\@<=\<',
            \ }

highlight def link perlFunctionTag Function
