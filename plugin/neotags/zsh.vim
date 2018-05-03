runtime! 'plugin/neotags/sh.vim'

if !exists('g:neotags#zsh#order')
	let g:neotags#zsh#order = g:neotags#sh#order
endif

let g:neotags#zsh#f = g:neotags#sh#f
let g:neotags#zsh#a = g:neotags#sh#a
