if !exists('g:neotags#vim#order')
	let g:neotags#vim#order = 'acfv'
endif

let g:neotags#vim#v = { 'group': 'VariableTag' }
let g:neotags#vim#a = { 'group': 'AutoGroupTag' }
let g:neotags#vim#c = {
            \   'group': 'CommandTag',
            \   'prefix': '\(\(^\|\s\):\?\)\@<=',
            \   'suffix': '\(!\?\(\s\|$\)\)\@='
            \ }

" Use :set iskeyword+=: for vim to make s:/<sid> functions to show correctly
let g:neotags#vim#f = {
            \   'group': 'FuncNameTag',
            \   'prefix': '\%(\<s:\|<[sS][iI][dD]>\)\@<!\<',
            \   'filter': {
            \       'pattern': '(?i)(<sid>|\bs:)',
            \       'group': 'vimScriptFuncNameTag',
            \       'prefix': '\C\%(\<s:\|<[sS][iI][dD]>\)',
            \   }
            \ }


highlight def link FuncNameTag		neotags_FunctionTag
highlight def link ScriptFuncNameTag	neotags_FunctionTag
highlight def link CommandTag		neotags_PreProcTag
highlight def link AutoGroupTag		neotags_PreProcTag
highlight def link VariableTag		neotags_VariableTag
