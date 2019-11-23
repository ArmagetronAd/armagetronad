ARG BASE_LINUX=amd64/alpine:3.7
ARG PROGRAM_NAME=armagetronad
ARG PROGRAM_TITLE="Armagetron Advanced"

FROM ${BASE_LINUX} AS download
MAINTAINER Manuel Moos <z-man@users.sf.net>

# download zthread and patch
WORKDIR /root/
RUN mkdir download && mkdir src
RUN wget https://forums3.armagetronad.net/download/file.php?id=9628 -O download/zthread.patch.bz2
ENV ZTHREAD_VER=2.3.2
RUN wget https://sourceforge.net/projects/zthread/files/ZThread/${ZTHREAD_VER}/ZThread-${ZTHREAD_VER}.tar.gz/download -O download/ZThread-${ZTHREAD_VER}.tar.gz

########################################

FROM download AS build_base

# build dependencies
RUN apk add \
autoconf \
automake \
bison \
boost-dev \
boost-thread \
glew-dev \
freetype-dev \
ftgl-dev \
git \
g++ \
make \
libpng-dev \
libxml2-dev \
protobuf-dev \
python2 \
sdl-dev \
sdl_image-dev \
sdl_mixer-dev \
sdl2-dev \
sdl2_image-dev \
sdl2_mixer-dev \
--no-cache

########################################

FROM build_base as zbuild

# build zthread
WORKDIR /root/
RUN cd src && tar -xzf ../download/ZThread-${ZTHREAD_VER}.tar.gz \
&& cd ZThread-${ZTHREAD_VER} && bzcat ../../download/zthread.patch.bz2 | patch -p 1 \
&& CXXFLAGS="-fpermissive -DPTHREAD_MUTEX_RECURSIVE_NP=PTHREAD_MUTEX_RECURSIVE" ./configure --prefix=/usr --enable-shared=false \
&& make install -j$(nproc)

########################################

# pack up result
From build_base AS build
COPY --from=zbuild /usr/include/zthread /usr/include/
COPY --from=zbuild /usr/bin/zthread* /usr/bin/
COPY --from=zbuild /usr/lib/libZ* /usr/lib/

########################################

# bootstrap source
FROM build AS bootstrap
ENV SOURCE_DIR /root/armagetronad_src
WORKDIR ${SOURCE_DIR}
COPY . ${SOURCE_DIR}
RUN ./bootstrap.sh

########################################

# build server
FROM bootstrap AS build_server

ARG PROGRAM_NAME
ARG PROGRAM_TITLE

RUN bash ./configure --prefix=/usr/local --disable-glout --disable-sysinstall --disable-desktop progname="${PROGRAM_NAME}" progtitle="${PROGRAM_TITLE}"
RUN make -j$(nproc)
RUN DESTDIR=/root/destdir make install

########################################

# build client
FROM bootstrap AS build_client

ARG PROGRAM_NAME
ARG PROGRAM_TITLE

RUN bash ./configure --prefix=/usr/local --disable-sysinstall --disable-desktop progname="${PROGRAM_NAME}" progtitle="${PROGRAM_TITLE}"
RUN make -j$(nproc)
RUN DESTDIR=/root/destdir make install

########################################

FROM ${BASE_LINUX} AS run_server_base
MAINTAINER Manuel Moos <z-man@users.sf.net>

# runtime dependencies
RUN apk add \
boost-thread \
libxml2 \
protobuf \
python2 \
shadow \
--no-cache

########################################

FROM run_server_base AS run_server
MAINTAINER Manuel Moos <z-man@users.sf.net>

ARG PROGRAM_NAME

WORKDIR /
COPY --from=build_server /root/destdir/ /
RUN sh /usr/local/share/games/${PROGRAM_NAME}-dedicated/scripts/sysinstall install /usr/local

