if !exists('g:neotags#perl#order')
	let g:neotags#perl#order = 'cps'
endif

let g:neotags#perl#c = { 'group': 'perlConstantTag' }
let g:neotags#perl#p = { 'group': 'perlPackageTag' }
let g:neotags#perl#s = {
            \   'group': 'perlFunctionTag',
            \   'prefix': '\%(\<sub\s\*\)\@<!\%(>\|\s\|&\|^\)\@<=\<',
            \ }

highlight def link perlConstantTag	neotags_ConstantTag
highlight def link perlFunctionTag	neotags_FunctionTag
highlight def link perlPackageTag	neotags_ModuleTag
