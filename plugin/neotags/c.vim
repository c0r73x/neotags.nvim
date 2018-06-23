if !exists('g:neotags#c#order')
        let g:neotags#c#order = 'guesmfdtv'
endif

" let g:neotags#c#g = { 'group': 'cEnumTypeTag' }
" let g:neotags#c#t = { 'group': 'cTypeTag' }

let g:neotags#c#g = {
            \ 'group': 'cEnumTypeTag',
            \ 'prefix': '\%(enum\s\+\)\@5<=',
            \ }
let g:neotags#c#s = {
            \   'group': 'cStructTag',
            \   'prefix': '\%(struct\s\+\)\@7<='
            \ }
let g:neotags#c#u = {
            \ 'group': 'cUnionTag',
            \ 'prefix': '\%(union\s\+\)\@6<=',
            \ }

let g:neotags#c#e = { 'group': 'cEnumTag' }
let g:neotags#c#d = { 'group': 'cPreProcTag' }

let g:neotags#c#m = {
            \   'group': 'cMemberTag',
            \   'prefix': '\%(\%(\>\|\]\|)\)\%(\.\|->\)\)\@5<=',
            \ }

" let g:neotags#c#f = {
            " \   'group': 'cFunctionTag',
            " \   'suffix': '\>\%(\s*(\)\@='
            " \ }
let g:neotags#c#f = { 'group': 'cFunctionTag' }

" let g:neotags#c#f = {
"             \   'group': 'cFunctionTag',
"             \   'suffix': '\>\%(\.\|->\)\@!'
"             \ }

let g:neotags#c#t = {
            \ 'group': 'cTypeTag',
            \ 'suffix': '\>\%(\.\|->\)\@!'
            \ }

let g:neotags#c#R = {
            \    'group': 'cFuncRef',
            \    'prefix': '\%(&\)\@1<='
            \ }

let g:neotags#c#v = { 'group': 'cGlobalVar' }

let g:neotags#c#equivalent = { 'p': 'f' }

highlight def link cClassTag	neotags_TypeTag
highlight def link cEnumTypeTag	neotags_EnumTypeTag
highlight def link cStructTag	neotags_StructTag
highlight def link cUnionTag	neotags_UnionTag
highlight def link cFuncRef	neotags_FunctionTag

highlight def link cGlobalVar	neotags_GlobalVarTag
highlight def link cEnumTag	neotags_EnumTag
highlight def link cFunctionTag	neotags_FunctionTag
highlight def link cMemberTag	neotags_MemberTag
highlight def link cPreProcTag	neotags_PreProcTag
highlight def link cTypeTag	neotags_TypeTag
