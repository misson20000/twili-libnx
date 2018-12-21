# libtwili

This is a small library for interacting with Twili stdio through libnx.

## Installation

```
$ cd /tmp
$ mkdir twili-libnx
$ cd twili-libnx
$ wget https://github.com/misson20000/twili-libnx/blob/master/PKGBUILD
$ makepkg -si
```

## Usage

Add `-ltwili` to your Makefile, before `-lnx`.
```
LIBS   := -ltwili -lnx
```

Include `<twili.h>`, call `twiliInitialize()` in your initializer, call `twiliExit()` in your finalizer, don't use `consoleDebugInit`, and enjoy.
