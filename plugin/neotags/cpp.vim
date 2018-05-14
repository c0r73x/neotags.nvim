if !exists('g:neotags#cpp#order')
        let g:neotags#cpp#order = 'cgstuedfm'
endif

let g:neotags#cpp#c = { 'group': 'ClassTag' }
let g:neotags#cpp#g = { 'group': 'EnumTypeTag' }
let g:neotags#cpp#s = { 'group': 'StructTag' }
let g:neotags#cpp#t = { 'group': 'TypeTag' }
let g:neotags#cpp#u = { 'group': 'UnionTag' }
let g:neotags#cpp#e = { 'group': 'EnumTag' }
let g:neotags#cpp#d = { 'group': 'PreProcTag' }
let g:neotags#cpp#m = {
            \   'group': 'MemberTag',
            \   'prefix': '\%(\%(\>\|\]\|)\)\%(\.\|->\)\)\@5<=',
            \ }
let g:neotags#cpp#f = {
            \   'group': 'FunctionTag',
            \   'suffix': '\>\%(\s*(\)\@='
            \ }

let g:neotags#c#equivalent = { 'p': 'f' }

highlight def link ClassTag	neotags_TypeTag
highlight def link EnumTypeTag	neotags_EnumTypeTag
highlight def link StructTag	neotags_StructTag
highlight def link UnionTag	neotags_UnionTag

highlight def link EnumTag	neotags_EnumTag
highlight def link FunctionTag	neotags_FunctionTag
highlight def link MemberTag	neotags_MemberTag
highlight def link PreProcTag	neotags_PreProcTag
highlight def link TypeTag	neotags_TypeTag
