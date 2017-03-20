# neotags.nvim

A neovim plugin that generates and highlight ctags similar to easytags.

I wrote this because I didn't like that my vim froze when opening large
projects.

## Requirements

neotags requires Neovim with if\_python3, and psutil for Python3.
If `:echo has("python3")` returns `1` and `pip3 list | grep psutil` shows the psutil package, then you're done; otherwise, see below.

You can enable Python3 interface and psutil with pip:

    pip3 install neovim psutil

## Commands

| Command | Description |
| ------ | ----------- |
| NeotagsToggle | Toggle neotags on the fly |

## Options

| Option | Description | Default |
| ------ | ----------- | ------- |
| g:neotags_enabled | Option to enable/disable neotags | 0 |
| g:neotags_file | Path to where to store the ctags file | ./tags |
| g:neotags_events_update | List of vim events when to run tag generation and update highlight | BufWritePost |
| g:neotags_events_highlight | List of vim events when to update highlight | BufReadPost |
| g:neotags_run_ctags | Option to enable/disable ctags generation from neotags | 1 |
| g:neotags_highlight | Option to enable/disable neotags highlighting | 1 |
| g:neotags_recursive | Option to enable/disable recursive tag generation | 1 |
| g:neotags_appendpath | Option to append current path to ctags arguments | 1 |
| g:neotags_ctags_bin | Location of ctags | ctags |
| g:neotags_ctags_args | ctags arguments | --fields=+l --c-kinds=+p --c++-kinds+p --sort=no --extra=+q |
| g:neotags_ctags_timeout | ctags timeout in seconds | 3 |
| g:neotags_silent_timeout | Hide message when ctags timeouts | 0 |
| g:neotags_verbose | Verbose output when reading and generating tags (for debug) | 0 |

## Tips

To use the_silver_searcher or similar applications when generating tags you can do something like this.

```viml
let g:neotags_appendpath = 0
let g:neotags_recursive = 0

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
