ARG BASE_ALPINE=docker.io/alpine:3.12
ARG CONFIGURE_ARGS=""
ARG FAKERELEASE=false
ARG PROGNAME="armagetronad"
ARG PROGTITLE="Armagetron Advanced"

########################################

# runtime prerequisites
FROM ${BASE_ALPINE} AS runtime_base
LABEL maintainer="Manuel Moos <z-man@users.sf.net>"

ARG PROGNAME

RUN apk add \
bash \
libxml2 \
libgcc \
libstdc++ \
--no-cache

# for 0.4
# boost-thread \
# protobuf \

WORKDIR /

########################################

# development prerequisites
FROM runtime_base AS builder

# build dependencies
RUN apk add \
autoconf \
automake \
patch \
bash \
bison \
g++ \
make \
libxml2-dev \
python3 \
--no-cache

# for 0.4
#protobuf-dev \
#boost-dev \
#boost-thread \

# download, patch, configure, build, install ZThread and remove all source traces in a single layer
RUN mkdir src && wget https://forums3.armagetronad.net/download/file.php?id=9628 -O src/zthread.patch.bz2 && \
wget https://sourceforge.net/projects/zthread/files/ZThread/2.3.2/ZThread-2.3.2.tar.gz/download -O src/zthread.tgz && \
cd src && tar -xzf zthread.tgz && cd ZThread* && bzcat ../zthread.patch.bz2 | patch -p 1 && \
CXXFLAGS="-fpermissive -DPTHREAD_MUTEX_RECURSIVE_NP=PTHREAD_MUTEX_RECURSIVE" ./configure --prefix=/usr --enable-shared=yes --enable-static=no && \
make -j `nproc` && \
make install && \
cd ../../ && rm -r src

#RUN find /usr/lib/ -name *ZThread*

########################################

# build
FROM builder as build

ARG CONFIGURE_ARGS
ARG FAKERELEASE
ARG PROGNAME
ARG PROGTITLE

ENV SOURCE_DIR /root/${PROGNAME}
ENV BUILD_DIR /root/build

COPY . ${SOURCE_DIR}
WORKDIR ${SOURCE_DIR}

RUN (test -r configure && test -f missing) || (./bootstrap.sh && cat version.m4)

RUN mkdir -p ${BUILD_DIR} && chmod 755 ${BUILD_DIR}
WORKDIR ${BUILD_DIR}
RUN ARMAGETRONAD_FAKERELEASE=${FAKERELEASE} progname="${PROGNAME}" progtitle="${PROGTITLE}" \
${SOURCE_DIR}/configure --prefix=/usr/local --disable-glout --disable-sysinstall --disable-useradd \
    --disable-master --disable-uninstall --disable-desktop \
    ${CONFIGURE_ARGS} && \
make -j `nproc` && \
DESTDIR=/root/destdir make install && \
rm -rf ${SOURCE_DIR} ${BUILD_DIR}

########################################

# finish runtime
FROM runtime_base AS runtime

COPY --chown=root --from=builder /usr/lib/*ZThread*.so* /usr/lib/

# pack
FROM runtime AS run_server

ARG PROGNAME

COPY --chown=root --from=build /root/destdir /

RUN sh /usr/local/share/games/*-dedicated/scripts/sysinstall install /usr/local && \
echo -e "#!/bin/bash\n \
/usr/local/bin/${PROGNAME}-dedicated \
--userdatadir /usr/share/${PROGNAME} \
--vardir /var/${PROGNAME} \
--autoresourcedir /var/${PROGNAME}/resource \
\"\$@\"" > /usr/local/bin/run.sh && \
chmod 755 /usr/local/bin/run.sh && \
mkdir -p /var/${PROGNAME} && chmod 777 /var/${PROGNAME}

USER nobody:nobody

VOLUME ["/var/${PROGNAME}"]
ENTRYPOINT ["/usr/local/bin/run.sh"]
EXPOSE 4534/udp

# Dump for launch script testing lines
#ls -al /var\n \
#ls -al /var/${PROGNAME}-dedicated\n \
#bash\n \
