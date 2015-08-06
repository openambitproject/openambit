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
#    docker run --rm -v $PWD:/code -u $(id -u) openambit:jessie
#
#  Doing so gives you a basic sanity check of code compilability on a
#  minimalistic, reproducible development platform.
#
#  If you don't like the defaults of building in $PWD/_build with no
#  options to either cmake or make, feel free to adjust the relevant
#  environment variables.  For example, you could build with:
#
#    docker run --rm -v $PWD:/code -u $(id -u) \
#      --env BUILD_DIR=tmp \
#      --env CMAKE_OPTS="-DBUILD_EXTRAS=1" \
#      --env MAKE_OPTS=-k \
#      openambit:jessie
#
#  Check out the --env-file option to docker if you find that overly
#  long-winded.  If you use a BUILD_DIR that is not below /code make
#  sure to drop the --rm option.
#
#  For interactive sessions, you may want to use:
#
#    docker run -i -t --rm -v $PWD:/code -u $(id -u) \
#      openambit:jessie /bin/bash
#
#  Again, drop the --rm option if you build in a location that is not
#  below /code.

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
ENV      BUILD_DIR _build
CMD      test -d ${BUILD_DIR} || mkdir ${BUILD_DIR}; \
	      cd ${BUILD_DIR} \
	      && cmake ${CMAKE_OPTS} .. \
	      && make ${MAKE_OPTS}

# Finally, things that really should be fixed in the Openambit code.
# FIXME add multiarch support to src/libambit/cmake/FindUdev.cmake
RUN  cd /usr/lib/ && ln -s x86_64-linux-gnu/libudev.so
