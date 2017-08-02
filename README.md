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

| Command       | Description               |
|---------------|---------------------------|
| NeotagsToggle | Toggle neotags on the fly |

## Options

| Option                     | Description                                                        | Default                                                      |
|----------------------------|--------------------------------------------------------------------|--------------------------------------------------------------|
| g:neotags_enabled          | Option to enable/disable neotags                                   | 0                                                            |
| g:neotags_file             | Path to where to store the ctags file                              | ./tags                                                       |
| g:neotags_events_update    | List of vim events when to run tag generation and update highlight | BufWritePost                                                 |
| g:neotags_events_highlight | List of vim events when to update highlight                        | BufReadPost                                                  |
| g:neotags_run_ctags        | Option to enable/disable ctags generation from neotags             | 1                                                            |
| g:neotags_highlight        | Option to enable/disable neotags highlighting                      | 1                                                            |
| g:neotags_recursive        | Option to enable/disable recursive tag generation                  | 1                                                            |
| g:neotags_appendpath       | Option to append current path to ctags arguments                   | 1                                                            |
| g:neotags_ctags_bin        | Location of ctags                                                  | ctags                                                        |
| g:neotags_ctags_args       | ctags arguments                                                    | --fields=+l --c-kinds=+p --c++-kinds+p --sort=no --extras=+q |
| g:neotags_ctags_timeout    | ctags timeout in seconds                                           | 3                                                            |
| g:neotags_silent_timeout   | Hide message when ctags timeouts                                   | 0                                                            |
| g:neotags_verbose          | Verbose output when reading and generating tags (for debug)        | 0                                                            |
| g:neotags#c#order          | Group Name creation for the C language                             | cedfm                                                        |
| g:neotags#cpp#order        | Group Name creation for the Cpp language                           | cedfm                                                        |
| g:neotags#python#order     | Group Name creation for the Python language                        | mfc                                                          |
| g:neotags#ruby#order       | Group Name creation for the Ruby language                          | mfc                                                          |
| g:neotags#sh#order         | Group Name creation for the Shell language                         | f                                                            |
| g:neotags#java#order       | Group Name creation for the Java language                          | cim                                                          |
| g:neotags#javascript#order | Group Name creation for the Javascript language                    | f                                                            |
| g:neotags#vim#order        | Group Name creation for the Vimscript language                     | acf                                                          |
| g:neotags#perl#order       | Group Name creation for the Perl language                          | s                                                            |
| g:neotags#php#order        | Group Name creation for the Php language                           | fc                                                           |


## Highlight Group Names
By default group name creation is set for all the different group names of all the supported languages. However, no 
highlighting command is set for any group name. Anyhow this is very easy to configure. Example configuration for
c\cpp:
```vim
let g:neotags#cpp#order = 'ced'
let g:neotags#c#order = 'ced'

highlight link cTypeTag Special
highlight link cppTypeTag Special
highlight link cEnumTag Identifier
highlight link cppEnumTag Identifier
highlight link cPreProcTag PreProc
highlight link cppPreProcTag PreProc
```
### C/Cpp
Default Highlight all groups:
```vim
let g:neotags#cpp#order = 'cedfm'
let g:neotags#c#order = 'cedfm'
```
| Option | Group Name     |
|--------|----------------|
| c      | cppTypeTag     |
| m      | cppMemberTag   |
| e      | cppEnumTag     |
| d      | cppPreProcTag  |
| f      | cppFunctionTag |

C highlighting is identical to Cpp just remove `pp` from the group name. Example, `cTypeTag`.
With the `g:neotags#cpp#order` function you can restrict the highlighting to selected groups. See `Speed Improvements`
below.

### Vimscript
```vim
let g:neotags#vim#order = 'acf'
```
| Option | Group Name      |
|--------|-----------------|
| a      | vimAutoGroupTag |
| c      | vimCommandTag   |
| f      | vimFuncNameTag  |

### Python
```vim
let g:neotags#python#order = 'mfc'
```
| Language | Group Name        |
|----------|-------------------|
| m        | pythonMethodTag   |
| f        | pythonFunctionTag |
| c        | pythonClassTag    |

### Ruby
```vim
let g:neotags#ruby#order = 'mfc'
```
| Option | Group Name        |
|--------|-------------------|
| m      | rubyModuleNameTag |
| f      | rubyClassNameTag  |
| c      | rubyMethodNameTag |

### Shell
```vim
let g:neotags#sh#order = 'f'
```
| Option | Group Name    |
|--------|---------------|
| f      | shFunctionTag |

### Java
```vim
let g:neotags#java#order = 'cim'
```
| Option | Group Name       |
|--------|------------------|
| c      | javaClassTag     |
| i      | javaInterfaceTag |
| m      | JavaMethodTag    |

### Javascript
```vim
let g:neotags#javascript#order = 'f'
```
| Option | Group Name            |
|--------|-----------------------|
| f      | javascriptFunctionTag |

### Perl
```vim
let g:neotags#perl#order = 's'
```
| Option | Group Name      |
|--------|-----------------|
| s      | perlFunctionTag |

### Php
```vim
let g:neotags#php#order = 'fc'
```
| Option | Group Name      |
|--------|-----------------|
| f      | phpFunctionsTag |
| c      | phpClassesTag   |

## Tips
To use `the_silver_searcher` or similar applications when generating tags you can do something like this.

```vim
let g:neotags_appendpath = 0
let g:neotags_recursive = 0

" Use this option for the_silver_searcher
let g:neotags_ctags_bin = 'ag -g "" '. getcwd() .' | ctags'
" Or this one for ripgrep. Not both.
let g:neotags_ctags_bin = 'rg --files '. getcwd() .' | ctags'
let g:neotags_ctags_args = [
            \ '-L -',
            \ '--fields=+l',
            \ '--c-kinds=+p',
            \ '--c++-kinds=+p',
            \ '--sort=no',
            \ '--extras=+q'
            \ ]
``` 
### Speed Improvements
Also on big projects syntax highlighting may become slow. To address this you can try:
```vim
set regexpengine=1
```
This provides significant speed improvements. In addition you set the highlight options for your language not
highlight everything but maybe only the tags your interested in the most. Example:
```vim
let g:neotags#cpp#order = 'ced'
```
The above will only highlight `cppTypeTag, cppPreProcTag, cppEnumTag`.
