Openambit
=========

Openambit makes your computer the Free (as in Freedom) conduit between
your Ambit watch and Suunto's `Movescount`_ site (if you want it to go
that far).  It enables you to get your hard-earned "move" log data off
*your* watch and onto *your* computer.  And if you really want it to,
Openambit will pump that data into the cloud where Big Data can crunch
it to pieces and analyze your moves to shreds.

Update
------

Suunto has discontinued the `Movescount` service, Openambit still allows
to fetch data from supported watches and also to update settings,
sport-modes and routes on the watch from local settings files.

See `Adjusting watch settings and routes without Movescount`_ for details 
about how to use Openambit without Movescount.

Modules
-------

Openambit includes the following modules:

src/libambit
  a library that let's your computer communicate with your Ambit watch.

src/openambit
  a Qt based GUI application to get data off your watch and push it to
  Suunto's `Movescount`_ site.

src/openambit-cli
  a commandline application which allows to automate fetching data
  from the watch or updating settings

src/openambit-routes
  a commandline application which can upload routes from local files.
  it expects the same format as Movescount uses via an information-file
  and a "points" file, referenced via the general settings file.

src/unittests
  a collection of tests which verify that parts of the code behave as
  expected.

src/example
  a very simple command-line application that reports on your watch's
  support status, and, if your watch is supported, battery charge and
  a summary of the moves on your watch.

tools
  contains a few utilities that people thought useful.  One compares
  Openambit's XML log files with those from `Moveslink2`_ and another
  converts the XML to `GPX`_.
  Finally `strava_upload.sh` allows to send data from Moves directly to
  Strava via the GPX file.

wireshark_dissector
  a `Wireshark`_ packet dissector to help reverse engineer the Ambit
  device protocol by picking your packet captures apart.


Source Or Binary?
-----------------

Let's be clear, building from the latest revision in the repository is
not quite for the faint of heart.  You need a development environment,
make sure build requirements are met, actually build (doh!) and pray
things work as intended.  And if worse comes to worst, you might even
brick your Ambit watch and turn it into a `paperweight`_.

Of course, we try our damnedest (and often put our own Ambits to the
test) to prevent that absolutely worst-case scenario but there are
*no* guarantees.

Distribution provided binary packages, such as provided by `Debian`_
and `Ubuntu`_ are normally built from reasonably well tested sources.
As in, it probably will not turn your Ambit into a paperweight.  That
might just meet your needs.  For other distributions, have a look at
`OSWatershed.org`_, or hit your favourite search engine.

If you cannot find binaries that meet your needs (not that unlikely at
present), you can compile from one of the source code archives on our
`release`_ page (or the corresponding ``git tag``).  That would also
be on the safe side.

As a last resort, or if you're a developer type, go ahead and build
from the latest, greatest "bleeding edge" version on the repository's
``master`` branch.


Requirements
------------

In order to build Openambit from source you need a couple of tools and
libraries.  To begin with, you will need ``cmake``, ``make`` and C and
C++ compilers.  You will need C and C++ libraries, Qt5 and ``zlib`` as
well as one of ``libudev`` and ``libusb-1.0``.
For all these libraries you will also need their header files
(typically provided in ``*-dev`` or ``*-devel`` packages).

On Debian/Ubuntu based distributions the following should install the
necessary packages:

``sudo apt-get install debhelper gcc g++ make cmake libusb-1.0-0-dev libudev-dev qtbase5-dev qttools5-dev-tools zlib1g-dev libpcap-dev libglib2.0-dev wireshark-dev qttools5-dev``

Openambit also makes use of the hidapi library from https://github.com/libusb/hidapi
The source is currently included, so no dependency needs to be installed
for it.

Build Procedure
---------------

The simplest way to build from source is by means of the ``build.sh``
script.  It will build all components needed to use Openambit.  Any
command-line arguments you specify are passed on to ``cmake``.  That
means you can set up a "Debug" build with:

.. code-block:: sh

   cd /path/to/your/clone/of/openambit
   ./build.sh -DCMAKE_BUILD_TYPE=Debug

Developer and otherwise inquisitive types may want to build some of
the extras that are included.  To do so:

.. code-block:: sh

   cd /path/to/your/clone/of/openambit
   BUILD_EXTRAS=1 ./build.sh

You can run the applications you built *without* installing as follows:

.. code-block:: sh

   ./run.sh			# runs the GUI application
   ./run.sh openambit		# runs the GUI application
   ./run.sh ambitconsole	# runs the example application

If you are only interested in building a selected module, you can just
use ``cmake`` directly.  For example, if all you really want to build
is ``libambit`` and nothing else, you could (for example):

.. code-block:: sh

   cd /path/to/your/clone/of/openambit
   mkdir _build
   cd _build
   cmake ../src/libambit
   make

Actually, you can also build everything directly with ``cmake``.  The
following ought to work:

.. code-block:: sh

   cd /path/to/your/clone/of/openambit
   mkdir _build
   cd _build
   cmake ..
   make
   
Build options

.. code-block:: sh

   BUILD_EXTRAS = 0 | 1 (Default 0)
   CMAKE_BUILD_TYPE = Debug | Release
   DEBUG_PRINT_INFO = 0 | 1 (Default 0)
   HIDAPI_DRIVER = libudev | libusb | pcapsimulate | mac | windows (Default <empty> => libudev)


Install Procedure
-----------------

If you have built from source with ``build.sh``, you can install with
``install.sh``.  This only installs the ``openambit`` application and
the ``libambit`` library it needs.  When you have built only selected
parts, simply install them with:

.. code-block:: sh

   cd /path/to/your/build/directory
   sudo make install

If building for a mac computer, consider installing the hidapi driver
and configure cmake for it:

.. code-block:: sh

  brew install hidapi
  cmake -DHIDAPI_DRIVER=mac

If you built directly with ``cmake``, installation is simply:

.. code-block:: sh

   cd /path/to/you/clone/of/openambit
   cd _build
   sudo make install

To enable the Wireshark dissector, just copy the ``ambit.so`` file to
your ``~/.wireshark/plugins/`` directory.  You can also put a symbolic
link there pointing to the build result so your next ``wireshark`` run
will use the latest and greatest(?) version.

Device setup
------------

In order to allow access to the device, you may need to do the following.

.. code-block:: sh

    sudo cp ./src/libambit/libambit.rules /etc/udev/rules.d/
    sudo udevadm control --reload-rules && udevadm trigger

This configures access to the device via udev.

.. _Movescount: http://www.movescount.com/
.. _Moveslink2: http://www.movescount.com/connect/moveslink/Suunto_Ambit
.. _GPX: https://en.wikipedia.org/wiki/GPS_Exchange_Format
.. _Wireshark: https://www.wireshark.org/
.. _paperweight: https://en.wikipedia.org/wiki/Paperweight
.. _Debian: https://packages.debian.org/search?keywords=openambit
.. _Ubuntu: http://packages.ubuntu.com/search?keywords=openambit
.. _OSWatershed.org: http://oswatershed.org/pkg/openambit
.. _release: https://github.com/openambitproject/openambit/releases
.. _Adjusting watch settings and routes without Movescount: https://github.com/openambitproject/openambit/wiki/Adjusting-watch-settings-and-routes-without-Movescount

More information
----------------

Look at the wiki at https://github.com/openambitproject/openambit/wiki for
more information.

