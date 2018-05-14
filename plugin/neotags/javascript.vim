let g:neotags#javascript#order = 'cCfmpo'

let g:neotags#javascript#c = {
            \   'group': 'ClassTag',
            \   'notin': [
            \       'jsx.*',
            \       'javascriptTemplate'
            \   ]
            \ }

let g:neotags#javascript#C = {
            \   'group': 'ConstantTag',
            \   'notin': [
            \       'jsx.*',
            \       'javascriptTemplate'
            \   ]
            \ }

let g:neotags#javascript#f = {
            \   'group': 'FunctionTag',
            \   'notin': [
            \       'jsx.*',
            \       'javascriptTemplate',
            \       'javascriptConditional',
            \       'javascriptRepeat'
            \   ]
            \ }

let g:neotags#javascript#m = {
            \   'group': 'MethodTag',
            \   'notin': [
            \       'jsx.*',
            \       'javascriptTemplate',
            \       'javascriptConditional',
            \       'javascriptRepeat'
            \   ]
            \ }

let g:neotags#javascript#o = {
            \   'group': 'ObjectTag',
            \   'notin': [
            \       'jsx.*',
            \       'javascriptTemplate',
            \       'javascriptConditional',
            \       'javascriptRepeat'
            \   ]
            \ }

let g:neotags#javascript#p = {
            \   'group': 'PropTag',
            \   'notin': [
            \       'jsx.*',
            \       'javascriptTemplate',
            \       'javascriptConditional',
            \       'javascriptRepeat'
            \   ]
            \ }

highlight def link ClassTag	neotags_ClassTag
highlight def link ConstantTag	neotags_ConstantTag
highlight def link FunctionTag	neotags_FunctionTag
highlight def link MethodTag	neotags_MethodTag
highlight def link ObjectTag	neotags_ObjectTag
highlight def link PropTag	neotags_PreProcTag
