# libtwili

This is a small library for interacting with Twili stdio through libnx.

## Installation

```
$ cd /tmp
$ mkdir twili-libnx
$ cd twili-libnx
$ wget https://github.com/misson20000/twili-libnx/raw/master/PKGBUILD
$ makepkg -si
```

Alternatively, this package is in the AUR as [libtwili](https://aur.archlinux.org/packages/libtwili/).

## Usage

Add `-ltwili` to your Makefile, before `-lnx`.
```
LIBS   := -ltwili -lnx
```

- Include `<twili.h>`.
- Call `twiliInitialize()` in your initializer. Remember to always check for errors.
- Call `twiliExit()` in your finalizer.
- For monitored process standard I/O, use `twiliBindStdio()`.
- See [twili.h](https://github.com/misson20000/twili-libnx/blob/master/include/twili.h) for more.
