if !exists('g:neotags#cpp#order')
        let g:neotags#cpp#order = 'cgstuedfm'
endif

let g:neotags#cpp#c = { 'group': 'cppClassTag' }
let g:neotags#cpp#g = { 'group': 'cppEnumTypeTag' }
let g:neotags#cpp#s = { 'group': 'cppStructTag' }
let g:neotags#cpp#t = { 'group': 'cppTypeTag' }
let g:neotags#cpp#u = { 'group': 'cppUnionTag' }
let g:neotags#cpp#e = { 'group': 'cppEnumTag' }
let g:neotags#cpp#d = { 'group': 'cppPreProcTag' }
let g:neotags#cpp#m = {
            \   'group': 'cppMemberTag',
            \   'prefix': '\%(\%(\>\|\]\|)\)\%(\.\|->\)\)\@5<=',
            \ }
let g:neotags#cpp#f = {
            \   'group': 'cppFunctionTag',
            \   'suffix': '\>\%(\s*(\)\@='
            \ }

let g:neotags#cpp#equivalent = { 'p': 'f' }

highlight def link cppClassTag		neotags_ClassTag
highlight def link cppEnumTypeTag	neotags_EnumTypeTag
highlight def link cppStructTag		neotags_StructTag
highlight def link cppUnionTag		neotags_UnionTag

highlight def link cppEnumTag		neotags_EnumTag
highlight def link cppFunctionTag	neotags_FunctionTag
highlight def link cppMemberTag		neotags_MemberTag
highlight def link cppPreProcTag	neotags_PreProcTag
highlight def link cppTypeTag		neotags_TypeTag
