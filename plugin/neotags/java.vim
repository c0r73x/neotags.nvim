if !exists('g:neotags#java#order')
    let g:neotags#java#order = 'cimegf'
endif

let g:neotags#java#c = { 'group': 'javaClassTag' }
let g:neotags#java#i = { 'group': 'javaInterfaceTag' }
let g:neotags#java#m = { 'group': 'javaMethodTag' }
let g:neotags#java#e = { 'group': 'javaEnumTag' }
let g:neotags#java#g = { 'group': 'javaEnumTypeTag' }
let g:neotags#java#f = { 'group': 'javaFieldTag' }


highlight def link ClassTag	neotags_TypeTag
highlight def link EnumTag	neotags_EnumTag
highlight def link EnumTypeTag	neotags_EnumTypeTag
highlight def link FieldTag	neotags_FieldTag
highlight def link InterfaceTag	neotags_InterfaceTag
highlight def link MethodTag	neotags_MethodTag
