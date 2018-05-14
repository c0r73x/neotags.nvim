let g:neotags#javascript#order = 'cCfmpo'

let g:neotags#javascript#c = {
            \   'group': 'javascriptClassTag',
            \   'notin': [
            \       'jsx.*',
            \       'javascriptTemplate'
            \   ]
            \ }

let g:neotags#javascript#C = {
            \   'group': 'javascriptConstantTag',
            \   'notin': [
            \       'jsx.*',
            \       'javascriptTemplate'
            \   ]
            \ }

let g:neotags#javascript#f = {
            \   'group': 'javascriptFunctionTag',
            \   'notin': [
            \       'jsx.*',
            \       'javascriptTemplate',
            \       'javascriptConditional',
            \       'javascriptRepeat'
            \   ]
            \ }

let g:neotags#javascript#m = {
            \   'group': 'javascriptMethodTag',
            \   'notin': [
            \       'jsx.*',
            \       'javascriptTemplate',
            \       'javascriptConditional',
            \       'javascriptRepeat'
            \   ]
            \ }

let g:neotags#javascript#o = {
            \   'group': 'javascriptObjectTag',
            \   'notin': [
            \       'jsx.*',
            \       'javascriptTemplate',
            \       'javascriptConditional',
            \       'javascriptRepeat'
            \   ]
            \ }

let g:neotags#javascript#p = {
            \   'group': 'javascriptPropTag',
            \   'notin': [
            \       'jsx.*',
            \       'javascriptTemplate',
            \       'javascriptConditional',
            \       'javascriptRepeat'
            \   ]
            \ }

highlight def link javascriptFunctionTag Function
highlight def link javascriptMethodTag Function
highlight def link javascriptObjectTag Identifier
highlight def link javascriptConstantTag PreProc
highlight def link javascriptPropTag PreProc
highlight def link javascriptClassTag Type
