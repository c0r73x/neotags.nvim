let g:neotags#zig#order = 'fTe'

let g:neotags#zig#e = {
            \   'group': 'zigClassTag',
            \ }

let g:neotags#zig#f = {
            \   'group': 'zigFunctionTag',
            \ }

let g:neotags#zig#T = {
            \   'group': 'zigTypeTag',
            \ }

highlight def link zigClassTag        neotags_ClassTag
highlight def link zigStructTag       neotags_StructTag
highlight def link zigFunctionTag     neotags_FunctionTag
highlight def link zigVariableTag     neotags_VariableTag
highlight def link zigExtensionTag    neotags_ConstantTag
highlight def link zigEnumTag         neotags_EnumTag
highlight def link zigMemberTag       neotags_MemberTag
highlight def link zigTypeTag         neotags_TypeTag
