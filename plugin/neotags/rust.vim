if !exists('g:neotags#rust#order')
        let g:neotags#rust#order = 'nsicgtmePfM'
endif

let g:neotags#rust#n = { 'group': 'rustModuleTag' }
let g:neotags#rust#s = { 'group': 'rustStructTag' }
let g:neotags#rust#i = { 'group': 'rustTraitInterfaceTag' }
let g:neotags#rust#c = { 'group': 'rustImplementationTag' }
let g:neotags#rust#g = { 'group': 'rustEnumTag' }
let g:neotags#rust#t = { 'group': 'rustTypeTag' }
let g:neotags#rust#m = { 'group': 'rustMemberTag' }
let g:neotags#rust#e = { 'group': 'rustEnumTypeTag' }
" let g:neotags#rust#P = { 'group': 'rustMethodTag' }
let g:neotags#rust#f = { 'group': 'rustFunctionTag' }
let g:neotags#rust#M = { 'group': 'rustMacroTag' }

let g:neotags#rust#equivalent = { 'P': 'f' }

highlight def link rustImplementationTag	neotags_TypeTag
highlight def link rustTraitInterfaceTag	neotags_TypeTag
highlight def link rustStructTag		neotags_TypeTag
highlight def link rustUnionTag			neotags_UnionTag
highlight def link rustEnumTypeTag		neotags_EnumTypeTag
highlight def link rustModuleTag		neotags_PreProcTag

highlight def link rustEnumTag		neotags_EnumTag
highlight def link rustFunctionTag	neotags_FunctionTag
highlight def link rustMemberTag	neotags_MemberTag
highlight def link rustMacroTag		neotags_PreProcTag
highlight def link rustTypeTag		neotags_TypeTag
