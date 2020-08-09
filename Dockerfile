ARG BASE_BUILD_SMALL=registry.gitlab.com/armagetronad/armagetronad/armalpine_32:028_0
ARG BASE_BUILD_FULL=registry.gitlab.com/armagetronad/armagetronad/armabuild_64:028_0
ARG BASE_LINUX=i386/alpine:3.7
ARG PROGRAM_NAME=armagetronad-unk
ARG PROGRAM_TITLE="Armagetron UNK"
ARG FAKERELEASE=false
ARG BRANCH=trunk-fix-unknown-bug

########################################

# bootstrap source
FROM ${BASE_BUILD_FULL} AS bootstrap
MAINTAINER Manuel Moos <z-man@users.sf.net>

ENV SOURCE_DIR /home/docker/armagetronad
ENV BUILD_DIR /home/docker/build

COPY --chown=docker . ${SOURCE_DIR}
RUN chmod 755 ${SOURCE_DIR}
WORKDIR ${SOURCE_DIR}
# these files are in .dockerignore, but if they're in git, restore them.
RUN test -d .git && git checkout .dockerignore .gitlab-ci.yml Dockerfile
RUN git status
#RUN ./batch/make/version .
#RUN false
RUN test -r configure || ./bootstrap.sh
RUN cat version.m4
#RUN false

########################################

# build tarball
FROM bootstrap AS configured

#ARG PROGRAM_NAME
#ARG PROGRAM_TITLE
ARG FAKERELEASE
ARG BRANCH

RUN mkdir -p ${BUILD_DIR} && chmod 755 ${BUILD_DIR}
WORKDIR ${BUILD_DIR}
RUN . ../armagetronad/docker/scripts/brand.sh . && ARMAGETRONAD_FAKERELEASE=${FAKERELEASE} ../armagetronad/configure --prefix=/usr/local --disable-glout --disable-sysinstall --disable-desktop progname="${PROGRAM_NAME}" progtitle="${PROGRAM_TITLE}"
RUN make -j$(nproc) dist && make -C docker/build tag.gits
RUN if [ ${FAKERELEASE} = true ]; then cp ../armagetronad/docker/build/fakerelease_proto.sh docker/build/fakerelease.sh; fi

########################################

# build server
FROM bootstrap AS build_server

ARG PROGRAM_NAME
ARG PROGRAM_TITLE

RUN bash ./configure --prefix=/usr/local --disable-glout --disable-sysinstall --disable-desktop progname="${PROGRAM_NAME}" progtitle="${PROGRAM_TITLE}"
RUN make -j$(nproc)
RUN DESTDIR=/home/docker/destdir make install

########################################

# build client
FROM bootstrap AS build_client

ARG PROGRAM_NAME
ARG PROGRAM_TITLE

RUN bash ./configure --prefix=/usr/local --disable-sysinstall --disable-desktop progname="${PROGRAM_NAME}" progtitle="${PROGRAM_TITLE}"
RUN make -j$(nproc)
RUN DESTDIR=/home/docker/destdir make install

########################################

FROM ${BASE_LINUX} AS run_server_base
MAINTAINER Manuel Moos <z-man@users.sf.net>

# runtime dependencies
RUN apk add \
boost-thread \
libxml2 \
protobuf \
python \
shadow \
--no-cache

########################################

FROM run_server_base AS run_server
MAINTAINER Manuel Moos <z-man@users.sf.net>

ARG PROGRAM_NAME

WORKDIR /
COPY --chown=root --from=build_server /home/docker/destdir/ /
RUN sh /usr/local/share/games/${PROGRAM_NAME}-dedicated/scripts/sysinstall install /usr/local

