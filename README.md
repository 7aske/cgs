# cgs

## Description

`cgs` (check-git-status) is a simple tool for us forgetful people. Namely what the tool does is checks a folder with git repositories for ones that have been changed but not committed. All that to avoid unnecessary merges and conflicts.

## Dependencies

Git - obviously

Unix based system

## Usage

```
cgs <flags>
```
By default `cgs` assumes that repositories are categorized in one of the following way in a root folder as show by the tree below.

```
root_folder
├── category1
│   ├── repository1
│   ├── repository2
│   ├── repository3
│   ├── repository4
│   └── repository5
├── category2
│   ├── repository1
│   ├── repository2
│   ├── repository3
│   ├── repository4
│   ├── repository5
│   └── repository6
├── repository1
└── repository2
```

Repository must be either the first sub-folder in the root folder or the first sub-folder of a categorizing folder (whether it was sorted by language or anything else). Root folder is being determined by the `CODE` environmental variable and it is recommended to have it set up in your `.bashrc` file as follows:

```
export CODE=/path/to/your/repos/folder
```

Otherwise it will not work as it was designed to require minimal typing in the actual command line to work.

Output of the program is "Everything is up to date" if there are no commits to be made or a listing of all repositories and their respective categorizing folders:

```
category1             repo5
category4             repo1
repo1
repo3
```

### Additional flags

Few options are available that pretty print the stats:

`-l` - shows all categories and number of repositories in them. Below that are listed all the uncommitted repositories.

```
category1           2  1
category2           5
category3          22  3
category4          12
category5           3

category1          repo1  
category3          repo2  
category3          repo5  
category3          repo8  
```

`-ll` - shows all categories and their repositories in a tree like form (same as `tree` linux program) with uncommitted ones being highlighted in red:

```
.
├── category1 (5)
│   ├── repository1
│   ├── repository2
│   ├── repository3
│   ├── repository4
│   └── repository5
├── category2 (6)
│   ├── repository1
│   ├── repository2
│   ├── repository3
│   ├── repository4
│   ├── repository5
│   └── repository6
├── repository1 (1)
└── repository2 (1)
```

`--no-color` - available to disable color output for usage in other command parsing programs like `exec`/`execp` command in `conky`.

## Build

You can build the program however you like just remember to link `pthread`. There is a `Makefile` with predefined recipes and you can just run them with:

```
make install
```

Make symlinks the `./build/cgs` into `/usr/bin`.
 