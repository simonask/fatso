# Fatso Package Manager

Fatso is a bundling package manager for compiled projects written in C. Think [Bundler](http://bundler.io/) for C/C++ projects.

Fatso downloads and builds all dependencies of your project, and installs them in a locally contained folder, so your project
can build and link against them with zero system dependencies.

Fatso supports the following build systems:

- Autotools
- CMake (TODO)
- Plain GNU Make (TODO)
- SCons (TODO)
- bjam (TODO)
- Custom scripts

Fatso is still in very early stages of development.

## Getting Started

Create a `fatso.yml` in your project. It's just a basic YAML-file specifying
your dependencies, and it could look like this:

```
project: my_project
version: 0.0.1
author: Simon Ask Ulsnes <simon@ulsnes.dk>
dependencies:
- ['libyaml', '~> 0.1.4']
```

The above project has a single dependency: `libyaml`, at fuzzy version 0.1.14.

To install the dependencies after having modified `fatso.yml`, run:

```
$ fatso install
```

To build your project, run:

```
$ fatso build
```

Fatso will automatically detect the toolchain your project is using. If it fails
to detect it, or your project needs special care while building, you have two
options:

1. Tell Fatso about it, by setting `toolchain: <build commands>` in your
   `fatso.yml`. Fatso will set the CFLAGS/LDFLAGS environment variables with
   the necessary options to build against your local Fatso environment, so if
   your build tool takes those into account, you should be fine.

2. Integrate Fatso into your existing build scripts by letting them call
   `fatso cflags` and `fatso ldflags` in the relevant phases. Both commands
   print the CFLAGS/LDFLAGS needed to build against your project's Fatso
   environment.


## Frequently Asked Questions

### Will package XYZ be supported by Fatso?

Hard to say. Most "well-behaved" software packages, i.e. packages that build
according to the usual conventions and don't assume that they're installed
in system-wide directories, should be trivial to support. Less well behaved
packages may require special patches or upstream support to be buildable in an
isolated environment.

### Are Fatso projects relocatable?

That depends if all your dependencies are relocatable. To be certain, run
`fatso clean` and `fatso install` after moving your project.


## Dependencies

- POSIX system (Linux, OS X)
- GNU Make or compatible
- C99 compiler (Clang or GCC should both work)
- [libyaml](http://pyyaml.org/wiki/LibYAML)
- [Git](http://git-scm.com/) (`git` tool must be in $PATH)
- [curl](http://curl.haxx.se) (`curl` tool must be in $PATH)


## Building

Run `make`.

Run `make test` to build and run tests.


## Windows Support

Windows support is not a priority at the moment, and design decisions will not be made with non-POSIX systems in mind. However,
patches to accommodate POSIX layers on Windows such as Cygwin will be accepted.
