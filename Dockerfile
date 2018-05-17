FROM debian

MAINTAINER Olof Hagsand <kristofer.hallin@cloudmon360.com>

ENV DEBIAN_FRONTEND noninteractive

# Packages needed for building
RUN apt-get update && apt-get install -y \
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
    sysstat

# Create a directory to hold source-code, dependencies etc
RUN mkdir /grideye
WORKDIR /grideye

# Clone all dependencies and grideye-agent
RUN git clone https://github.com/olofhagsand/cligen.git
RUN git clone https://github.com/clicon/clixon.git

# Build cligen
WORKDIR /grideye/cligen
RUN ./configure
RUN make
RUN make install

# Build clixon
WORKDIR /grideye/clixon
RUN git checkout -b develop origin/develop
RUN ./configure
RUN make
RUN make install
RUN make install-include

# Build grideye-agent
RUN mkdir /grideye/grideye_agent
WORKDIR /grideye/grideye_agent
ADD . /grideye/grideye_agent
RUN ./configure
RUN make
RUN make install

WORKDIR /grideye/grideye_agent/plugins/
RUN make install

RUN rm -rf /grideye
RUN ldconfig

# Create a file for disk IO tests
RUN /bin/dd if=/dev/zero of=/var/tmp/GRIDEYE_LARGEFILE bs=1M count=1K

# Run grideye-agent
CMD if [ -z "$NOSSL" ]; then grideye_agent -u $GRIDEYE_URL -I $GRIDEYE_UUID -N $GRIDEYE_NAME -F -s; else grideye_agent -u $GRIDEYE_URL -I $GRIDEYE_UUID -N $GRIDEYE_NAME -F -s -s; fi
