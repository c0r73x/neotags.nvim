if !exists('g:neotags#vim#order')
	let g:neotags#vim#order = 'acfv'
endif

let g:neotags#vim#v = { 'group': 'vimVariableTag' }
let g:neotags#vim#a = { 'group': 'vimAutoGroupTag' }
let g:neotags#vim#c = {
            \   'group': 'vimCommandTag',
            \   'prefix': '\(\(^\|\s\):\?\)\@<=',
            \   'suffix': '\(!\?\(\s\|$\)\)\@='
            \ }

" Use :set iskeyword+=: for vim to make s:/<sid> functions to show correctly
" let g:neotags#vim#f = { 'group': 'vimFuncNameTag', }
let g:neotags#vim#f = {
            \ 'group': 'vimFuncNameTag',
            \ 'prefix': '\%(\%(g\|s\|l\):\)\=',
            \ }
" let g:neotags#vim#f = {
"             \   'group': 'vimFuncNameTag',
"             \   'prefix': '\%(\<s:\|<[sS][iI][dD]>\)\@<!\<',
"             \   'filter': {
"             \       'pattern': '(?i)(<sid>|\bs:)',
"             \       'group': 'vimvimScriptFuncNameTag',
"             \       'prefix': '\C\%(\<s:\|<[sS][iI][dD]>\)\?',
"             \   }
"             \ }


highlight def link vimFuncNameTag		neotags_FunctionTag
highlight def link vimScriptFuncNameTag	neotags_FunctionTag
highlight def link vimCommandTag		neotags_PreProcTag
highlight def link vimAutoGroupTag		neotags_PreProcTag
highlight def link vimVariableTag		neotags_VariableTag
