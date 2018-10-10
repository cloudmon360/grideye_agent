#!/bin/sh

# Install dependencies
apt-get update && apt-get install -y \
    curl \
    git \
    make \
    gcc \
    automake \
    flex \
    bison \
    libcurl4-openssl-dev \
    libfcgi-dev \
    python3-dev \
    libpython3-dev \
    sysstat \
    iperf3

# Clone repos
mkdir grideye_build

BASEDIR="`pwd`/grideye_build"

cd $BASEDIR
git clone https://github.com/clicon/clixon.git
git clone https://github.com/olofhagsand/cligen.git
git clone https://github.com/cloudmon360/grideye_agent.git

cd $BASEDIR/cligen
./configure
make
make install

cd $BASEDIR/clixon
git checkout -b develop origin/develop
./configure
make
make install
make install-include

cd $BASEDIR/grideye_agent
./configure
make
make install

cd $BASEDIR/grideye_agent/plugins/
make
make install
