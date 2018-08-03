if !exists('g:neotags#java#order')
    let g:neotags#java#order = 'cimegf'
endif

let g:neotags#java#c = { 'group': 'javaClassTag' }
let g:neotags#java#i = { 'group': 'javaInterfaceTag' }
let g:neotags#java#m = { 'group': 'javaMethodTag' }
let g:neotags#java#e = { 'group': 'javaEnumTag' }
let g:neotags#java#g = { 'group': 'javaEnumTypeTag' }
let g:neotags#java#f = { 'group': 'javaFieldTag' }


highlight def link javaClassTag	neotags_ClassTag
highlight def link javaEnumTag	neotags_EnumTag
highlight def link javaEnumTypeTag	neotags_EnumTypeTag
highlight def link javaFieldTag	neotags_FieldTag
highlight def link javaInterfaceTag	neotags_InterfaceTag
highlight def link javaMethodTag	neotags_MethodTag
