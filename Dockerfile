#
# Stage 1
#

FROM alpine

# Packages needed for building
RUN apk update && apk add \
    glib \
    curl \
    curl-dev \
    git \
    make \
    gcc \
    g++ \
    automake \
    flex \
    bison \
    fcgi-dev \
    python3-dev \
    sysstat

# RUN ldconfig

# Create a directory to hold source-code, dependencies etc
RUN mkdir /grideye
RUN mkdir /grideye/build

WORKDIR /grideye

# Clone all dependencies and grideye-agent
RUN git clone https://github.com/olofhagsand/cligen.git
RUN git clone https://github.com/clicon/clixon.git

# Build cligen
WORKDIR /grideye/cligen

RUN ./configure --prefix=/grideye/build
RUN make
RUN make install

# Build clixon
WORKDIR /grideye/clixon

RUN git checkout -b develop origin/develop
RUN ./configure --with-cligen=/grideye/build/ --prefix=/grideye/build/
RUN make
RUN make install
RUN make install-include

# Build grideye-agent
RUN mkdir /grideye/grideye_agent
WORKDIR /grideye/grideye_agent
ADD . /grideye/grideye_agent
RUN autoconf
RUN ./configure --prefix=/grideye/build/ --with-cligen=/grideye/build

RUN make
RUN make install

WORKDIR /grideye/grideye_agent/plugins/
RUN make install

# RUN ldconfig

#
# Stage 2
#

FROM alpine
MAINTAINER Kristofer Hallin <kristofer.hallin@cloudmon360.com>
ENV DEBIAN_FRONTEND noninteractive

# Packages needed for building
RUN apk update && apk add \
    curl \
    make \
    libcurl \
    fcgi \
    python3 \
    sysstat \
    iperf3

RUN pip3 install iperf3
    
COPY --from=0 /grideye/build/bin/ /usr/local/bin/
COPY --from=0 /grideye/build/etc/ /usr/local/etc/
COPY --from=0 /grideye/build/include/ /usr/local/include/
COPY --from=0 /grideye/build/lib/ /usr/local/lib/
COPY --from=0 /grideye/build/sbin/ /usr/local/sbin/
COPY --from=0 /grideye/build/share/ /usr/local/share/

RUN ls /usr/local/bin/

#RUN ldconfig

# Run grideye-agent
CMD if [ -z "$NOSSL" ]; then /usr/local/bin/grideye_agent -P /usr/local/lib/grideye/agent/ -u $GRIDEYE_URL -I $GRIDEYE_UUID -N $GRIDEYE_NAME -F -s; else /usr/local/bin/grideye_agent -P /usr/local/lib/grideye/agent/ -u $GRIDEYE_URL -I $GRIDEYE_UUID -N $GRIDEYE_NAME -F -s -s; fi
