if !exists('g:neotags#c#order')
        let g:neotags#c#order = 'gstuedfm'
endif

let g:neotags#c#g = { 'group': 'cEnumTypeTag' }
let g:neotags#c#s = {
            \   'group': 'cStructTag',
            \   'prefix': '\%(struct\s\+\)\@7<='
            \ }
let g:neotags#c#t = { 'group': 'cTypeTag' }
let g:neotags#c#u = { 'group': 'cUnionTag' }
let g:neotags#c#e = { 'group': 'cEnumTag' }
let g:neotags#c#d = { 'group': 'cPreProcTag' }
let g:neotags#c#m = {
            \   'group': 'cMemberTag',
            \   'prefix': '\%(\%(\>\|\]\|)\)\%(\.\|->\)\)\@5<=',
            \ }
let g:neotags#c#f = {
            \   'group': 'cFunctionTag',
            \   'suffix': '\>\%(\s*(\)\@='
            \ }

let g:neotags#c#equivalent = { 'p': 'f' }

highlight def link cClassTag	neotags_TypeTag
highlight def link cEnumTypeTag	neotags_EnumTypeTag
highlight def link cStructTag	neotags_StructTag
highlight def link cUnionTag	neotags_UnionTag

highlight def link cEnumTag	neotags_EnumTag
highlight def link cFunctionTag	neotags_FunctionTag
highlight def link cMemberTag	neotags_MemberTag
highlight def link cPreProcTag	neotags_PreProcTag
highlight def link cTypeTag	neotags_TypeTag
