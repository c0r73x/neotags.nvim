let g:neotags#haxe#order = 'cfvet'

let g:neotags#haxe#c = {
            \   'group': 'haxeClassTag',
            \ }

let g:neotags#haxe#f = {
            \   'group': 'haxeFunctionTag',
            \ }

let g:neotags#haxe#v = {
            \   'group': 'haxeVariableTag',
            \   'allow_keyword': 0,
            \ }

let g:neotags#haxe#e = {
            \   'group': 'haxeEnumTag',
            \ }

let g:neotags#haxe#t = {
            \   'group': 'haxeTypeTag',
            \ }

highlight def link haxeClassTag        neotags_ClassTag
highlight def link haxeStructTag       neotags_StructTag
highlight def link haxeFunctionTag     neotags_FunctionTag
highlight def link haxeVariableTag     neotags_VariableTag
highlight def link haxeExtensionTag    neotags_ConstantTag
highlight def link haxeEnumTag         neotags_EnumTag
highlight def link haxeMemberTag       neotags_MemberTag
highlight def link haxeTypeTag         neotags_TypeTag
