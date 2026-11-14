fulleditor: A full editing plugin for RedAlien
====================================================================================================

`fulleditor` is a plugin for RedAlien that provides a full editing experience using an editor
registered with the `EDITOR` environment variable.


Usage
----------------------------------------------------------------------------------------------------

```console
fulleditor -f PATH
fulleditor (-h|--help)
fulleditor (-v|--version)
```

### Required arguments

* `-f`, `--file`: The path to the plugin input/output file. If the file does not exist, the current
                  command line content is regarded as empty. If the file is not writable, `omnipicker`
                  exits with an error.

### Optional arguments

* `-h`, `--help`: Show the help message and exit.
* `-v`, `--version`: Show the version and exit.


License
----------------------------------------------------------------------------------------------------

`fulleditor` is distributed as part of RedAlien and is licensed under the same terms.

