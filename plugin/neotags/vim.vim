if !exists('g:neotags#vim#order')
	let g:neotags#vim#order = 'acf'
endif

let g:neotags#vim#a = {
            \   'group': 'vimAutoGroupTag'
            \ }

let g:neotags#vim#c = {
            \   'group': 'vimCommandTag',
            \   'prefix': '\(\(^\|\s\):\?\)\@<=',
            \   'suffix': '\(!\?\(\s\|$\)\)\@='
            \ }

let g:neotags#vim#f = {
            \   'group': 'vimFuncNameTag',
            \   'prefix': '\%(\<s:\|<[sS][iI][dD]>\)\@<!\<',
            \   'filter': {
            \       'pattern': '(?i)(<sid>|\bs:)',
            \       'group': 'vimScriptFuncNameTag',
            \       'prefix': '\C\%(\<s:\|<[sS][iI][dD]>\)',
            \   }
            \ }

highlight def link vimFuncNameTag Function
highlight def link vimScriptFuncNameTag Function
highlight def link vimCommandTag PreProc
highlight def link vimAutoGroupTag Define
