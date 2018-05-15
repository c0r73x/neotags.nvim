if !exists('g:neotags#go#order')
        let g:neotags#go#order = 'pftsicm'
endif

let g:neotags#go#p = { 'group': 'goPackageTag', }
let g:neotags#go#c = { 'group': 'goConstantTag', }
let g:neotags#go#t = { 'group': 'goTypeTag', }
let g:neotags#go#s = { 'group': 'goStructTag', }
let g:neotags#go#i = { 'group': 'goInterfaceTag', }
let g:neotags#go#f = {
            \   'group': 'goFunctionTag',
            \   'suffix': '\>\%(\s*(\)\@='
            \ }
let g:neotags#go#m = {
            \   'group': 'goMemberTag',
            \   'prefix': '\%(\%(\>\|\]\|)\)\.\)\@5<='
            \ }

highlight def link goConstantTag	neotags_ConstantTag
highlight def link goFunctionTag	neotags_FunctionTag
highlight def link goInterfaceTag	neotags_InterfaceTag
highlight def link goMemberTag	neotags_MemberTag
highlight def link goPackageTag	neotags_PreProcTag
highlight def link goStructTag	neotags_StructTag
highlight def link goTypeTag	neotags_TypeTag
