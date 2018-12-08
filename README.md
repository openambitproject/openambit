# Openambit


Openambit makes your computer the Free (as in Freedom) conduit between
your Ambit watch and Suunto's [Movescount](http://www.movescount.com/)
site (if you want it to go that far). It enables you to get your
hard-earned "move" log data off *your* watch and onto *your* computer.
And if you really want it to, Openambit will pump that data into the
cloud where Big Data can crunch it to pieces and analyze your moves to
shreds.

## Modules


Openambit includes the following modules:

src/libambit

:   a library that let's your computer communicate with your Ambit
    watch.

src/openambit

:   a Qt based GUI application to get data off your watch and push it to
    Suunto's [Movescount](http://www.movescount.com/) site.

src/example

:   a very simple command-line application that reports on your watch's
    support status, and, if your watch is supported, battery charge and
    a summary of the moves on your watch.

tools

:   contains a few utilities that people thought useful. One compares
    Openambit's XML log files with those from
    [Moveslink2](http://www.movescount.com/connect/moveslink/Suunto_Ambit)
    and another converts the XML to
    [GPX](https://en.wikipedia.org/wiki/GPS_Exchange_Format).

wireshark\_dissector

:   a [Wireshark](https://www.wireshark.org/) packet dissector to help
    reverse engineer the Ambit device protocol by picking your packet
    captures apart.

## Installation

### Source Or Binary?


Currently there are no binaries for linux. Hence building from source is
the only option.

Let's be clear, building from the latest revision in the repository is
not quite for the faint of heart. You need a development environment,
make sure build requirements are met, actually build (doh!) and pray
things work as intended. And if worse comes to worst, you might even
brick your Ambit watch and turn it into a
[paperweight](https://en.wikipedia.org/wiki/Paperweight).

Of course, we try our damnedest (and often put our own Ambits to the
test) to prevent that absolutely worst-case scenario but there are *no*
guarantees.

Distribution provided binary packages, such as provided by
[Debian](https://packages.debian.org/search?keywords=openambit) and
[Ubuntu](http://packages.ubuntu.com/search?keywords=openambit) are
normally built from reasonably well tested sources. As in, it probably
will not turn your Ambit into a paperweight. That might just meet your
needs. For other distributions, have a look at
[OSWatershed.org](http://oswatershed.org/pkg/openambit), or hit your
favourite search engine.

If you cannot find binaries that meet your needs (not that unlikely at
present), you can compile from one of the source code archives on our
[release](https://github.com/openambitproject/openambit/releases) page
(or the corresponding `git tag`). That would also be on the safe side.

As a last resort, or if you're a developer type, go ahead and build from
the latest, greatest "bleeding edge" version on the repository's
`master` branch.

### Short installation instructions for debian and ubuntu


Install dependecies:

```sh
sudo apt-get install build-essential cmake git libudev-dev libqjson-dev libgusb-dev qtbase5-dev qtbase5-dev-tools qtbase5-private-dev qttools5-dev qttools5-dev-tools qttools5-private-dev qtchooser qt5-qmake
```

Get source and install:

```sh
git clone https://github.com/openambitproject/openambit
cd openambit
./install.sh
```

Run:

```sh
openambit
```

Possible issues:

-   "Device not ready": Reconnect watch after installation
-   "movescount.so.0 not found": rebuild lib cache: ```$ sudo ldconfig -v```


## Building

### Requirements

In order to build Openambit from source you need a couple of tools and
libraries. To begin with, you will need `cmake`, `make` and C and C++
compilers. You will need C and C++ libraries, Qt5 and `zlib` as well as
one of `libudev` and `libusb-1.0`. For all these libraries you will also
need their header files (typically provided in `*-dev` or `*-devel`
packages).

### Build Procedure

The simplest way to build from source is by means of the `build.sh`
script. It will build all components needed to use Openambit. Any
command-line arguments you specify are passed on to `cmake`. That means
you can set up a "Debug" build with:

```sh
cd /path/to/your/clone/of/openambit
./build.sh -DCMAKE_BUILD_TYPE=Debug
```

Developer and otherwise inquisitive types may want to build some of the
extras that are included. To do so:

```sh
cd /path/to/your/clone/of/openambit
BUILD_EXTRAS=1 ./build.sh
```

You can run the applications you built *without* installing as follows:

```sh
./run.sh         # runs the GUI application
./run.sh openambit       # runs the GUI application
./run.sh ambitconsole    # runs the example application
```

If you are only interested in building a selected module, you can just
use `cmake` directly. For example, if all you really want to build is
`libambit` and nothing else, you could (for example):

```sh
cd /path/to/your/clone/of/openambit
mkdir _build
cd _build
cmake ../src/libambit
make
```

Actually, you can also build everything directly with `cmake`. The
following ought to work:

```sh
cd /path/to/your/clone/of/openambit
mkdir _build
cd _build
cmake ..
make
```

Build options

```sh
BUILD_EXTRAS = 0 | 1 (Default 0)
CMAKE_BUILD_TYPE = Debug | Release
DEBUG_PRINT_INFO = 0 | 1 (Default 0)
```

## Install Procedure

### Default

```sh
./install.sh
```

If you have built from source with `build.sh`, you can install with
`install.sh`. This only installs the `openambit` application and the
`libambit` library it needs.

### Alternative
When you have built only selected parts,
simply install them with:

```sh
cd /path/to/your/build/directory
sudo make install
```

If you built directly with `cmake`, installation is simply:

```sh
cd /path/to/you/clone/of/openambit
cd _build
sudo make install
```

To enable the Wireshark dissector, just copy the `ambit.so` file to your
`~/.wireshark/plugins/` directory. You can also put a symbolic link
there pointing to the build result so your next `wireshark` run will use
the latest and greatest(?) version.

## Log files location

The log files downloaded from the watch are located in: ```~/.openambit```
