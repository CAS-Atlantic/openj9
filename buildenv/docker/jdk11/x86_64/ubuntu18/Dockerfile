# Copyright (c) 2018, 2019 IBM Corp. and others
#
# This program and the accompanying materials are made available under
# the terms of the Eclipse Public License 2.0 which accompanies this
# distribution and is available at https://www.eclipse.org/legal/epl-2.0/
# or the Apache License, Version 2.0 which accompanies this distribution and
# is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# This Source Code may also be made available under the following
# Secondary Licenses when the conditions for such availability set
# forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
# General Public License, version 2 with the GNU Classpath
# Exception [1] and GNU General Public License, version 2 with the
# OpenJDK Assembly Exception [2].
#
# [1] https://www.gnu.org/software/classpath/license.html
# [2] http://openjdk.java.net/legal/assembly-exception.html
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception

# To use this docker file:
#   docker build -t=openj9 .
#   docker run -it openj9

FROM ubuntu:18.04

# Install required OS tools
RUN apt-get update \
  && apt-get install -qq -y --no-install-recommends \
    software-properties-common \
  && apt-get update \
  && apt-get install -qq -y --no-install-recommends \
    autoconf \
    ca-certificates \
    ccache \
    cmake \
    cpio \
    file \
    git \
    git-core \
    libasound2-dev \
    libcups2-dev \
    libdwarf-dev \
    libelf-dev \
    libfontconfig1-dev \
    libfreetype6-dev \
    libnuma-dev \
    libx11-dev \
    libxext-dev \
    libxrender-dev \
    libxrandr-dev \
    libxt-dev \
    libxtst-dev \
    make \
    nasm \
    pkg-config \
    ssh \
    unzip \
    wget \
    zip \
    vim \
    build-essential \
    gdb \
    valgrind \
    libxext-dev \
    libxrender-dev \
  && rm -rf /var/lib/apt/lists/*

CMD [ "/bin/bash" ]