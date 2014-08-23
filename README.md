Fatso Package Manager
=====================

Fatso is a bundling package manager for compiled projects written in C. Think [Bundler](http://bundler.io/) for C/C++ projects.

Fatso downloads and builds all dependencies of your project, and installs them in a locally contained folder, so your project
can build and link against them with zero system dependencies.

Fatso supports the following build systems:

- TODO

Fatso is still in very early stages of development.


Dependencies
------------

- POSIX system (Linux, OS X)
- GNU Make or compatible
- C99 compiler (Clang or GCC should both work)
- [libyaml](http://pyyaml.org/wiki/LibYAML)
- [Git](http://git-scm.com/) (`git` tool must be in path)


Building
--------

Run `make`.

Run `make test` to build and run tests.


Windows Support
---------------

Windows support is not a priority at the moment, and design decisions will not be made with non-POSIX systems in mind. However,
patches to accommodate POSIX layers on Windows such as Cygwin will be accepted.
