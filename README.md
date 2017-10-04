### Haskell Support for Qt Creator

This Qt Creator plugin adds basic support for the Haskell programming language.

#### Features

* Code highlighting
* Editor tool tips with symbol information
* Follow symbol
* Basic .cabal project support
* Basic build configuration
* Basic run configuration

#### Requirements

The plugin currently only supports projects using [Haskell Stack](https://haskellstack.org).

The project must already be set up, if something mysteriously does not work check the following:

* The project's resolver must be installed. Ensure this by running `stack setup` in the project directory.
* For code info and navigation to work, `ghc-mod` is required to be built for the project's resolver. Ensure this by running `stack build ghc-mod` in the project directory.
* The plugin looks for the `stack` executable in the default installation directory of the Haskell Stack installers. If this is not correct for you, adapt the path in *Options* > *Haskell*.

Linux: Note that Haskell Stack from the Ubuntu distribution and probably others is hopelessly outdated. Use the installers provided by the [Haskell Stack](https://haskellstack.org) project.