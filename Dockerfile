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
boost-thread \
libxml2 \
libgcc \
libstdc++ \
protobuf \
runit \
--no-cache


WORKDIR /

########################################

# development prerequisites
FROM runtime_base AS builder

# build dependencies
RUN apk add \
autoconf \
automake \
boost-dev \
patch \
bash \
bison \
g++ \
make \
libxml2-dev \
protobuf-dev \
python3 \
--no-cache

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
    LDFLAGS=-Wl,-z,stack-size=8388608 \
    ${CONFIGURE_ARGS} && \
make -j `nproc` && \
DESTDIR=/root/destdir make install && \
rm -rf ${SOURCE_DIR} ${BUILD_DIR}

########################################

# pack
FROM runtime_base AS runtime
FROM runtime_base AS run_server

ARG PROGNAME

COPY --chown=root --from=build /root/destdir /
COPY batch/docker-entrypoint.sh.in /usr/local/bin/docker-entrypoint.sh

RUN sh /usr/local/share/games/*-dedicated/scripts/sysinstall install /usr/local && \
sed -i /usr/local/bin/docker-entrypoint.sh -e "s/@progname@/${PROGNAME}/g" && \
chmod 755 /usr/local/bin/docker-entrypoint.sh

USER nobody:nobody

VOLUME ["/var/${PROGNAME}"]
ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]
#ENTRYPOINT ["bash"]
EXPOSE 4534/udp
