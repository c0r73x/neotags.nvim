# neotags.nvim

A neovim plugin that generates and highlight ctags similar to easytags.

I wrote this because I didn't like that my vim froze when opening large
projects.


## Tips

To use the_silver_searcher or similar applications when generating tags you can do something like this.

```
let g:neotags_appendpath = 0

let g:neotags_ctags_bin = 'ag -g "" '. getcwd() .' | ctags'
let g:neotags_ctags_args = [
            \ '-L -',
            \ '--fields=+l',
            \ '--c-kinds=+p',
            \ '--c++-kinds=+p',
            \ '--sort=no',
            \ '--extra=+q'
            \ ]
``` 
