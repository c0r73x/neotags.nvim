# neotags.nvim

A neovim plugin that generates and highlight ctags similar to easytags.

I wrote this because I didn't like that my vim froze when opening large
projects.

Much of the plugin's speed boost comes from it's ability to filter tags at
runtime. Only tags found in opened buffers are supplied to Neovim for
highlighting. Easytags, on the other hand, supplied all tags found recursively
for a whole project. For very large projects with possibly tens of thousands of
tags, this made Neovim's parser grind to a halt. With the new method, the tag
list is far more manageable.

Be warned that for such large projects it can take some time to process the
list of tags. Fortunately, this is done asynchronously and does not slow the
editor. Furthermore, it is only updated when either a new file is opened or the
buffer is saved. This infrequent time cost is made in place of constantly
forcing Neovim to do the same processing.


![ScreenShot](/screenshot.gif?raw=true)

## Requirements

Neotags requires Neovim with if\_python3, and psutil for Python3.
If `:echo has("python3")` returns `1` and `pip3 list | grep psutil` shows the
psutil package, then you're done; otherwise, see below.

You can enable Python3 interface and psutil with pip:

    pip3 install neovim psutil

If tags processing is taking too long it may be advisable to use pypy3 in
place of python3. This is possible by adding `let g:python3_host_prog = 'pypy3'`
to your `.vimrc`.  Be advised that it will be necessary to have the Neovim
python module and any modules required by any other Vim plugins installed for
pypy3 as well (including psutil).

## Configuration

There are several configuration options to tweak the processing behavior. By
default, to speed things up all standard autotools files (such as configure,
Makefile.in, etc) are ignored by ctags. This behavior can be disabled with by
setting `g:neotags_no_autoconf` to `0`. Further filenames may be ignored by
appending `--exclude=foo` to the ctags argument list (see `man ctags` for more
information).

By default, a tags file is generated in a global directory for each directory
in which source files appear. If you would like to reduce the number of tags
files, and thereby conglomerate a project into one file, you may designate any
directory as a "project" top directory using the `NeotagsAddProject` command.
See below for details.

## Optional C Extension

Because the original implementation in python can take a modestly long time for
very large projects, the section of code that does the tag filtering has been
rewritten in C. If used, this can quite dramatically decrease the waiting time,
often by up to 4+ times.

It can be enabled simply by compiling it. Run `make` in the root git directory
and the small project will be automatically configured, compiled, and installed
into `~/.vim_tags/bin`. The requirements are cmake, libpcre2, and of course
a working C compiler. All of these should be readily available on any Unix like
platform. If you want to install it elsewhere feel free to configure and build
the project yourself. It is also possible to configure with autotools by
running the included `autogen.sh` if you really prefer.

The build process can be easily automated with a package manager such as dein.
Just add

    call dein#add('c0r73x/neotags.nvim', {'build': 'make'})

to your .vimrc or init.nvim file and dein will handle the rest. To disable it
after installing either delete the binary or add `let g:neotags_bin = ''` to
your setup.

As usual, on Windows things are more difficult. It is possible to compile with
MinGW, but the resulting binary is usually slower than the original python! If
you have Visual Studio installed then it is possible to generate a project with
cmake, provided that you can source a copy of `libpcre2-8.lib` or similar from
somewhere. There are no easily available pre-compiled version of this library,
so you'll either have to compile it yourself or download MinGW and use its
pre-compiled version (called `libpcre2-8.dll.a`). Put it in the same directory
as the top `CMakeFiles.txt` file and everything should work. There isn't any
shortcut around this, unfortunately.

If all of this seems like too much bother (especially for Windows users!) then
as mentioned the python version will work perfectly fine, and is probably
plenty fast enough for the majority of cases.

## Commands

| Command                             | Description                                                         |
| ----------------------------------- | ------------------------------------------------------
| `NeotagsToggle`                     | Toggle neotags on the fly                                           |
| `NeotagsAddProject <DIRECTORY>`     | Add a directory to the global list of "project" top directories     |
| `NeotagsRemoveProject <DIRECTORY`   | Remove a directry from the global list of "project" top directories |
| `NeotagsBinToggle`                  | Toggle usage of the compiled C binary                               |

## Options

| Option                         | Description                                                                                                            | Default                                                                                                                                                                                                      |
| ------------------------------ | --------------------------------------------------------------------                                                   | --------------------------------------------------------------                                                                                                                                               |
| g:neotags_enabled              | Option to enable/disable neotags                                                                                       | `0`                                                                                                                                                                                                          |
| g:neotags_directory            | Global directory in which to store all generated tags files                                                            | `~/.vim_tags`                                                                                                                                                                                                |
| g:neotags_settings_file        | Global file in which to store all saved "project" directories                                                          | `g:neotags_directory/neotags.json`                                                                                                                                                                           |
| g:neotags_ignored_tags         | List of tag names globally excluded from ever being highlighted (eg. try `NULL` in C)                                  | `""`                                                                                                                                                                                                         |
| g:neotags_no_autoconf          | Automatically exclude all standard GNU autotools files (except `Makefile`) to speed up processing by having fewer tags | `1`                                                                                                                                                                                                          |
| g:neotags_events_update        | List of vim events when to run tag generation and update highlight                                                     | `BufWritePost`                                                                                                                                                                                               |
| g:neotags_events_highlight     | List of vim events when to update highlight                                                                            | `BufEnter, BufReadPre`                                                                                                                                                                                       |
| g:neotags_events_rehighlight   | List of vim events when to clear cache and update highlight                                                            | `Syntax,` FileType                                                                                                                                                                                           |
| g:neotags_run_ctags            | Option to enable/disable ctags generation from neotags                                                                 | `1`                                                                                                                                                                                                          |
| g:neotags_highlight            | Option to enable/disable neotags highlighting                                                                          | `1`                                                                                                                                                                                                          |
| g:neotags_recursive            | Option to enable/disable recursive tag generation                                                                      | `1`                                                                                                                                                                                                          |
| g:neotags_find_tool            | Command (such as `ag -g`) run in place of `ctags -R` to find files                                                     | `""`                                                                                                                                                                                                         |
| g:neotags_ctags_bin            | Location of ctags                                                                                                      | `ctags`                                                                                                                                                                                                      |
| g:neotags_ctags_args           | ctags arguments                                                                                                        | `--fields=+l --c-kinds=+p --c++-kinds+p --sort=no --extras=+q`                                                                                                                                               |
| g:neotags_ctags_timeout        | ctags timeout in seconds                                                                                               | `3`                                                                                                                                                                                                          |
| g:neotags_silent_timeout       | Hide message when ctags timeouts                                                                                       | `0`                                                                                                                                                                                                          |
| g:neotags_verbose              | Verbose output (for debug, must be set before neotags is starated)                                                     | `0`                                                                                                                                                                                                          |
| g:neotags_ignore               | List of filetypes to ignore                                                                                            | `'text','nofile','mail','qf'`                                                                                                                                                                                |
| g:neotags_global_notin         | List of global syntax groups which should not include highlighting.                                                    | `'.&ast;String.&ast;', '.&ast;Comment.&ast;', 'cIncluded', 'cCppOut2', 'cCppInElse2', 'cCppOutIf2', 'pythonDocTest', 'pythonDocTest2'`                                                                       |
| g:neotags_ft_conv              | Dictionary of languages to convert between ctags and vim                                                               | `{ 'C++': 'cpp', 'C#': 'cs' }`                                                                                                                                                                               |
| g:neotags_ft_ext               | Dictionary of languages to convert between vim and file extensions                                                     | `{ 'python': ['py'], 'perl': ['pl', 'pm'], 'cpp': ['cpp', 'cxx', 'c', 'h', 'hpp'], 'c': ['c', 'h'], 'ruby': ['rb'], 'javascript': ['js', 'jsx', 'vue'], 'vue': ['js', 'vue'], 'typescript': ['ts', 'tsx'] }` |
| g:neotags_tagfiles_by_type     | Uses `g:neotags_regex_tool` and `g:neotags_find_tool` to only find files by extension(s)                               | `0`                                                                                                                                                                                                          |
| g:neotags_regex_tool           | Regex tool to use with `g:neotags_tagfiles_by_type`                                                                    | `ag`                                                                                                                                                                                                         |
| g:neotags#c#order              | Group Name creation for the C language                                                                                 | `cgstuedfpm`                                                                                                                                                                                                 |
| g:neotags#cpp#order            | Group Name creation for the Cpp language                                                                               | `cgstuedfpm`                                                                                                                                                                                                 |
| g:neotags#python#order         | Group Name creation for the Python language                                                                            | `mfc`                                                                                                                                                                                                        |
| g:neotags#ruby#order           | Group Name creation for the Ruby language                                                                              | `mfc`                                                                                                                                                                                                        |
| g:neotags#sh#order             | Group Name creation for the Shell language                                                                             | `fa`                                                                                                                                                                                                         |
| g:neotags#java#order           | Group Name creation for the Java language                                                                              | `cimegf`                                                                                                                                                                                                     |
| g:neotags#javascript#order     | Group Name creation for the Javascript language                                                                        | `cCfmpo`                                                                                                                                                                                                     |
| g:neotags#vim#order            | Group Name creation for the Vimscript language                                                                         | `acfv`                                                                                                                                                                                                       |
| g:neotags#perl#order           | Group Name creation for the Perl language                                                                              | `s`                                                                                                                                                                                                          |
| g:neotags#php#order            | Group Name creation for the Php language                                                                               | `cfdi`                                                                                                                                                                                                       |

## Highlight Group Names
By default group name creation is set for all the different group names of all the supported languages.

### C/Cpp
Default Highlight all groups:
```vim
let g:neotags#cpp#order = 'cgstuedfpm'
let g:neotags#c#order = 'cgstuedfpm'
```
| Option | Group Name     |
|--------|----------------|
| c      | cppTypeTag     |
| g      | cppTypeTag     |
| s      | cppTypeTag     |
| t      | cppTypeTag     |
| u      | cppTypeTag     |
| e      | cppEnumTag     |
| d      | cppPreProcTag  |
| f      | cppFunctionTag |
| p      | cppFunctionTag |
| m      | cppMemberTag   |

C highlighting is identical to Cpp just remove `pp` from the group name. Example, `cTypeTag`.
With the `g:neotags#cpp#order` function you can restrict the highlighting to selected groups. See `Speed Improvements`
below.

### Vimscript
```vim
let g:neotags#vim#order = 'acfv'
```
| Option | Group Name      |
|--------|-----------------|
| a      | vimAutoGroupTag |
| c      | vimCommandTag   |
| f      | vimFuncNameTag (Uses vimScriptFuncNameTag for local script functions)
| v      | vimVariableTag  |

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

### Shell / Zsh
```vim
let g:neotags#sh#order = 'fa'
```
| Option | Group Name    |
|--------|---------------|
| f      | shFunctionTag |
| a      | shAliasTag    |

### Java
```vim
let g:neotags#java#order = 'cimegf'
```
| Option | Group Name       |
|--------|------------------|
| c      | javaClassTag     |
| i      | javaInterfaceTag |
| m      | javaMethodTag    |
| e      | javaEnumTag      |
| g      | javaEnumTypeTag  |
| f      | javaFieldTag     |

### Javascript
```vim
let g:neotags#javascript#order = 'cCfmpo'
```
| Option | Group Name            |
|--------|-----------------------|
| c      | javascriptClassTag    |
| C      | javascriptConstantTag |
| f      | javascriptFunctionTag |
| m      | javascriptMethodTag   |
| p      | javascriptPropsTag    |
| o      | javascriptObjectTag   |

### Perl
```vim
let g:neotags#perl#order = 's'
```
| Option | Group Name      |
|--------|-----------------|
| s      | perlFunctionTag |

### Php
```vim
let g:neotags#php#order = 'cfdi'
```
| Option | Group Name      |
|--------|-----------------|
| c      | phpClassesTag   |
| f      | phpFunctionsTag |
| d      | phpConstantTag  |
| i      | phpInterfaceTag |
| a      | phpInterfaceTag |

## Tips
To use `the_silver_searcher` or similar applications when generating tags you can do something like this.

```vim
let g:neotags_recursive = 1

" Use this option for the_silver_searcher
let g:neotags_find_tool = 'ag -g ""'
" Or this one for ripgrep. Not both.
let g:neotags_find_tool = 'ag --files'
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

### ptags
Neotags have support for [ptags](https://github.com/dalance/ptags) by adding
let `g:neotags_ctags_bin = 'ptags'` to your vimrc.

Do note that if you have your own custom settings for `g:neotags_ctags_args` you
need to prepend these with -c. ~~Also since ptags do not support -L- the
`g:neotags_find_tool` will be ignored.~~ (-L was added in the latest version).

This is my setup using ptags for git repositories and ctags for other folders.

```vim
function s:neotagsCtagsCheck()
    if system('git rev-parse --is-inside-work-tree') =~# 'true'
        let g:neotags_ctags_bin = 'ptags'
        echom 'Neotags using ptags'
        let g:neotags_ctags_args = [
                    \   '-c --fields=+l',
                    \   '-c --c-kinds=+p',
                    \   '-c --c++-kinds=+p',
                    \   '-c --sort=yes',
                    \   "-c --exclude='.mypy_cache'",
                    \   '-c --regex-go=''/^\s*(var)?\s*(\w*)\s*:?=\s*func/\2/f/''',
                    \   "-c --exclude='*Makefile'",
                    \   "-c --exclude='*Makefile.in'",
                    \   "-c --exclude='*aclocal.m4'",
                    \   "-c --exclude='*config.guess'",
                    \   "-c --exclude='*config.h.in'",
                    \   "-c --exclude='*config.log'",
                    \   "-c --exclude='*config.status'",
                    \   "-c --exclude='*configure'",
                    \   "-c --exclude='*depcomp'",
                    \   "-c --exclude='*install-sh'",
                    \   "-c --exclude='*missing'",
                    \ ]
    else
        let g:neotags_ctags_bin = 'ctags'
        echom 'Neotags using ctags'
        let g:neotags_ctags_args = [
                    \   '--fields=+l',
                    \   '--c-kinds=+p',
                    \   '--c++-kinds=+p',
                    \   '--sort=yes',
                    \   "--exclude='.mypy_cache'",
                    \   '--regex-go=''/^\s*(var)?\s*(\w*)\s*:?=\s*func/\2/f/''',
                    \   "--exclude='*Makefile'",
                    \   "--exclude='*Makefile.in'",
                    \   "--exclude='*aclocal.m4'",
                    \   "--exclude='*config.guess'",
                    \   "--exclude='*config.h.in'",
                    \   "--exclude='*config.log'",
                    \   "--exclude='*config.status'",
                    \   "--exclude='*configure'",
                    \   "--exclude='*depcomp'",
                    \   "--exclude='*install-sh'",
                    \   "--exclude='*missing'",
                    \ ]
    endif
endfunction

augroup Neotags
    autocmd VimEnter * call s:neotagsCtagsCheck()
augroup END
```

### Language conversion

The `neotags_ft_conv` variable is used to convert for example C++ to cpp but it 
can be used to convert custom filetypes to ctag filetypes.

For example this is what i use for flow

```vim
let g:neotags_ft_conv = {
            \ 'C++': 'cpp',
            \ 'C#': 'cs',
            \ 'JavaScript': 'flow',
            \ }
```

Note that you do need to copy the javascript neotags file
`neotags.vim/plugin/neotags/javascript.vim` to `after/plugin/neotags/flow.vim`
and do a replace for '#javascript' to '#flow'

### Custom Rules

You can create custom rules for existing languages or new languages.

```
let g:neotags#[ctags language]#order = 'string with ctags kinds'
let g:neotags#[ctags language]#[ctags kind] = { 'group': 'highlight' }
```
For more advanced rules, check the files in `neotags.vim/plugin/neotags/*.vim`.

You can get the list of kinds by running `ctags --list-kinds=[language]`.

order determents priority of the highlight by first to last (tags with the same name will use the one with higher priority). Note that only kinds in the order string will be loaded.

For example, this is what I use in typescript/tsx

In ~/.ctags
```
--langdef=typescript
--langmap=typescript:.ts
--langmap=typescript:+.tsx
--regex-typescript=/^[ \t]*(export([ \t]+abstract)?([ \t]+default)?)?[ \t]*class[ \t]+([a-zA-Z0-9_]+)/\4/c,classes/
--regex-typescript=/^[ \t]*(declare)?[ \t]*namespace[ \t]+([a-zA-Z0-9_]+)/\2/n,modules/
--regex-typescript=/^[ \t]*(export)?[ \t]*module[ \t]+([a-zA-Z0-9_]+)/\2/M,modules/
--regex-typescript=/^[ \t]*(export)?[ \t]*function[ \t]+([a-zA-Z0-9_]+)/\2/f,functions/
--regex-typescript=/^[ \t]*export[ \t]+(var|let|const)[ \t]+([a-zA-Z0-9_]+)/\2/v,variables/
--regex-typescript=/^[ \t]*(var|let|const)[ \t]+([a-zA-Z0-9_]+)[ \t]*=[ \t]*function[ \t]*\(\)/\2/V,varlambdas/
--regex-typescript=/^[ \t]*(export)?[ \t]*(public|protected|private)[ \t]+(static)?[ \t]*([a-zA-Z0-9_]+)/\4/m,members/
--regex-typescript=/^[ \t]*(export)?[ \t]*interface[ \t]+([a-zA-Z0-9_]+)/\2/i,interfaces/
--regex-typescript=/^[ \t]*(export)?[ \t]*type[ \t]+([a-zA-Z0-9_]+)/\2/t,types/
--regex-typescript=/^[ \t]*(export)?[ \t]*enum[ \t]+([a-zA-Z0-9_]+)/\2/e,enums/
--regex-typescript=/^[ \t]*import[ \t]+([a-zA-Z0-9_]+)/\1/I,imports/
--regex-typescript=/^[ \t]*@([A-Za-z0-9._$]+)[ \t]*/\1/d,decorator/
```

In vimrc
```vim
let g:neotags#typescript#order = 'cnfmoited'

let g:neotags#typescript#c = { 'group': 'javascriptClassTag' }
let g:neotags#typescript#C = { 'group': 'javascriptConstantTag' }
let g:neotags#typescript#f = { 'group': 'javascriptFunctionTag' }
let g:neotags#typescript#o = { 'group': 'javascriptObjectTag' }

let g:neotags#typescript#n = g:neotags#typescript#C
let g:neotags#typescript#f = g:neotags#typescript#f
let g:neotags#typescript#m = g:neotags#typescript#f
let g:neotags#typescript#o = g:neotags#typescript#o
let g:neotags#typescript#i = g:neotags#typescript#C
let g:neotags#typescript#t = g:neotags#typescript#C
let g:neotags#typescript#e = g:neotags#typescript#C

let g:neotags#typescript#d = g:neotags#typescript#c
```
