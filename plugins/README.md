RedAlien plugins
====================================================================================================

This directory contains the source code and README files for the plugins that are included
by default in RedAlien. Each plugin is stored in its own dedicated subdirectory, which contains
a README file that explains information about the plugin, its functions, and how to use it.


omnipicker
----------------------------------------------------------------------------------------------------

`omnipicker` is a TUI-based selection tool of file paths, command histories, process IDs,
environment variables, and tokens from past commands. See [README.md](omnipicker/README.md)
for details.

### Usage example

```shell
omnipicker -m file -f /tmp/redalien_test.txt
```


fulleditor
----------------------------------------------------------------------------------------------------

`fulleditor` calls a full editor (registered in the `EDITOR` environment variable) to edit
the current command line. See [README.md](fulleditor/README.md) for details.

### Usage example

```shell
fulleditor -f /tmp/redalien_test.txt
```

