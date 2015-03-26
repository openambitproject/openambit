#  Dockerfile -- to build the sources on a well-known platform
#  Copyright (C) 2014, 2015  Olaf Meeuwissen
#
#  This file is part of Openambit.
#
#  Openambit is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published
#  by the Free Software Foundation, either version 3 of the License,
#  or (at your option) any later version.
#
#    Openambit distributed in the hope that it will be useful, but
#    WITHOUT ANY WARRANTY --- without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Openambit.  If not, see https://www.gnu.org/licenses/.

#  Recommended way to build the container this creates (for lack of
#  an easy way to exclude everything but the Dockerfile):
#
#    docker build -t openambit:jessie - < Dockerfile
#
#  After that is just a matter of compiling the sources with:
#
#    docker run -v $PWD:/code openambit:jessie
#
#  once you've removed any existing *-build directories.  In order
#  to set BUILD_EXTRAS and pass arguments on to `cmake` that becomes
#  something like:
#
#    docker run -v $PWD:/code --env BUILD_EXTRAS=1 openambit:jessie \
#      ./build.sh -DCMAKE_BUILD_TYPE=Debug
#
#  Doing so gives you a basic sanity check of code compilability on a
#  minimalistic, reproducible development platform.

FROM        debian:jessie
MAINTAINER  Olaf Meeuwissen <paddy-hack@member.fsf.org>

ENV  APT_OPTS --assume-yes --no-install-recommends

# build system dependencies
# Note that gcc does *not* depend on any specific C library.  Debian
# and derivatives ship several ...
RUN  apt-get update \
     && apt-get install ${APT_OPTS} \
                cmake \
                make \
                gcc \
                libc-dev

# libambit and example application build dependencies
# The HID API support needs at least one of these to be available.  The
# HIDAPI_DRIVER `cmake` variable controls what is used.
RUN  apt-get update \
     && apt-get install ${APT_OPTS} \
             libudev-dev \
             libusb-1.0-0-dev \
             libpcap-dev

# openambit build dependencies
# Note that libqjson-dev needs to be >= 0.8
RUN  apt-get update \
     && apt-get install ${APT_OPTS} \
             g++ \
	     libqjson-dev \
             libqt4-dev \
             zlib1g-dev

# wireshark dissector build dependencies
RUN  apt-get update \
     && apt-get install ${APT_OPTS} \
             libglib2.0-dev \
             libwireshark-dev \
             python

WORKDIR  /code
CMD      ./build.sh

# Finally, things that really should be fixed in the Openambit code.
# FIXME add multiarch support to src/libambit/cmake/FindUdev.cmake
RUN  cd /usr/lib/ && ln -s x86_64-linux-gnu/libudev.so
