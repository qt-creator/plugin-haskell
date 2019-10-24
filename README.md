# Haskell Support for Qt Creator

This Qt Creator plugin adds basic support for the Haskell programming language.

## Features

* Syntax highlighting
* Basic .cabal project support
* Basic build configuration
* Basic run configuration

Other editing features like code completion and navigation are provided via
[haskell-ide-engine](https://github.com/haskell/haskell-ide-engine) and Qt Creator's
Language Server Protocol client.

## Requirements

### Projects

The plugin currently only supports projects using [Haskell Stack](https://haskellstack.org).

* The plugin looks for the `stack` executable in the default installation directory of the Haskell
  Stack installers. If this is not correct for you, adapt the path in *Options* > *Haskell*.

Linux: Note that Haskell Stack from the Ubuntu distribution and probably others is hopelessly
outdated. Use the installers provided by the [Haskell Stack](https://haskellstack.org) project.

### Editing

Install [haskell-ide-engine](https://github.com/haskell/haskell-ide-engine) for the GHC version
that your project uses and [configure it](https://doc.qt.io/qtcreator/creator-language-servers.html)
in Qt Creator's language client:

* Open *Options* > *Language Client*
* Add a new server
* Set *Language* to the MIME types `text/x-haskell`, `text/x-haskell-project` and
  `text/x-literate-haskell`
* Set *Startup behavior* to *Start Server per Project*
* Set *Executable* to `hie-wrapper`, for example to `/home/myself/.local/bin/hie-wrapper`
* Set *Arguments* to `-lsp`

Note that HIE compiles your project before providing any information, so it might take some time.
