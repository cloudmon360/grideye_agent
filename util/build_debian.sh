#!/bin/sh

# Install dependencies
apt-get update
apt-get install -y libfcgi-dev curl sysstat libcurl4-nss-dev gcc make bison flex git

# Clone repos
mkdir grideye_build

BASEDIR="`pwd`/grideye_build"

cd $BASEDIR
git clone https://github.com/clicon/clixon.git
git clone https://github.com/olofhagsand/cligen.git
git clone https://github.com/cloudmon360/grideye_agent.git

cd $BASEDIR/clixon
git checkout -b develop origin/develop
./configure
make
make install
make install-include

cd $BASEDIR/cligen
./configure
make
make install

cd $BASEDIR/grideye_agent
./configure
sed -i -- 's/0 \/\* Verify/1 \/\* Verify/' grideye_agent.c
make
make install

cd $BASEDIR/grideye_agent/plugins/
make
make install
