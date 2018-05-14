if !exists('g:neotags#go#order')
        let g:neotags#go#order = 'pftsicm'
endif

let g:neotags#go#p = { 'group': 'PackageTag', }
let g:neotags#go#c = { 'group': 'ConstantTag', }
let g:neotags#go#t = { 'group': 'TypeTag', }
let g:neotags#go#s = { 'group': 'StructTag', }
let g:neotags#go#i = { 'group': 'InterfaceTag', }
let g:neotags#go#f = {
            \   'group': 'FunctionTag',
            \   'suffix': '\>\%(\s*(\)\@='
            \ }
let g:neotags#go#m = {
            \   'group': 'MemberTag',
            \   'prefix': '\%(\%(\>\|\]\|)\)\.\)\@5<='
            \ }

highlight def link ConstantTag	neotags_ConstantTag
highlight def link FunctionTag	neotags_FunctionTag
highlight def link InterfaceTag	neotags_InterfaceTag
highlight def link MemberTag	neotags_MemberTag
highlight def link PackageTag	neotags_PreProcTag
highlight def link StructTag	neotags_StructTag
highlight def link TypeTag	neotags_TypeTag
