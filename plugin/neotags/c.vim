if !exists('g:neotags#c#order')
        let g:neotags#c#order = 'gstuedfm'
endif

let g:neotags#c#g = { 'group': 'EnumTypeTag' }
let g:neotags#c#s = { 'group': 'StructTag' }
let g:neotags#c#t = { 'group': 'TypeTag' }
let g:neotags#c#u = { 'group': 'UnionTag' }
let g:neotags#c#e = { 'group': 'EnumTag' }
let g:neotags#c#d = { 'group': 'PreProcTag' }
let g:neotags#c#m = {
            \   'group': 'MemberTag',
            \   'prefix': '\%(\%(\>\|\]\|)\)\%(\.\|->\)\)\@5<=',
            \ }
let g:neotags#c#f = {
            \   'group': 'FunctionTag',
            \   'suffix': '\>\%(\s*(\)\@='
            \ }

let g:neotags#c#equivalent = { 'p': 'f' }

" highlight def link cEnumTypeTag	Type
" highlight def link cStructTag	Type
" highlight def link cUnionTag	Type
" 
" highlight def link cEnumTag	Define
" highlight def link cFunctionTag	Function
" highlight def link cMemberTag	Identifier
" highlight def link cPreProcTag	PreProc
" highlight def link cTypeTag	Type

highlight def link ClassTag	neotags_TypeTag
highlight def link EnumTypeTag	neotags_EnumTypeTag
highlight def link StructTag	neotags_StructTag
highlight def link UnionTag	neotags_UnionTag

highlight def link EnumTag	neotags_EnumTag
highlight def link FunctionTag	neotags_FunctionTag
highlight def link MemberTag	neotags_MemberTag
highlight def link PreProcTag	neotags_PreProcTag
highlight def link TypeTag	neotags_TypeTag
