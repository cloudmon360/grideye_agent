#
# Dockerfile for grideye-agent
#

FROM debian

MAINTAINER Olof Hagsand <olof@hagsand.se>

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
    libcurl4-openssl-dev

# Create a directory to hold source-code, dependencies etc
RUN mkdir /grideye
WORKDIR /grideye

# Clone all dependencies and grideye-agent
RUN git clone https://github.com/olofhagsand/cligen.git
RUN git clone https://github.com/clicon/clixon.git
RUN git clone https://github.com/olofhagsand/grideye_agent.git

# Build cligen
WORKDIR /grideye/cligen
RUN ./configure
RUN make
RUN make install

# Build clixon
WORKDIR /grideye/clixon
RUN ./configure --without-restconf --without-keyvalue
RUN make
RUN make install
RUN make install-include

# Build grideye-agent
WORKDIR /grideye/grideye_agent
RUN ./configure
RUN make
RUN make install

WORKDIR /

RUN ldconfig

# Run grideye-agent
CMD grideye_agent -u $GRIDEYE_URL -I $GRIDEYE_UUID -N $GRIDEYE_NAME -p udp :7878

EXPOSE 7878 7878/udp