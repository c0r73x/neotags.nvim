if !exists('g:neotags#php#order')
    let g:neotags#php#order = 'cfdi'
endif

let g:neotags#php#c = {
            \   'group': 'phpClassTag',
            \   'allow_keyword': 0,
            \ }

let g:neotags#php#d = {
            \   'group': 'phpConstantTag',
            \   'allow_keyword': 0,
            \ }

let g:neotags#php#i = {
            \   'group': 'phpInterfaceTag',
            \   'allow_keyword': 0,
            \ }

let g:neotags#php#f = {
            \   'group': 'phpFunctionsTag',
            \   'suffix': '(\@='
            \ }

let g:neotags#php#equivalent = { 'a': 'i' }

highlight def link phpClassTag     neotags_ClassTag
highlight def link phpConstantTag  neotags_ConstantTag
highlight def link phpInterfaceTag neotags_InterfaceTag
highlight def link phpFunctionsTag neotags_FunctionTag
