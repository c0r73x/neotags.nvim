if !exists('g:neotags#perl#order')
	let g:neotags#perl#order = 'cps'
endif

let g:neotags#perl#c = { 'group': 'ConstantTag' }
let g:neotags#perl#p = { 'group': 'PackageTag' }
let g:neotags#perl#s = {
            \   'group': 'FunctionTag',
            \   'prefix': '\%(\<sub\s\*\)\@<!\%(>\|\s\|&\|^\)\@<=\<',
            \ }

highlight def link ConstantTag	neotags_ConstantTag
highlight def link FunctionTag	neotags_FunctionTag
highlight def link PackageTag	neotags_ModuleTag
