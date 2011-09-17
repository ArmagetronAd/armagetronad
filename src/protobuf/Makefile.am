include $(top_srcdir)/Makefile.paths

# protocol buffer rules
%.pb: ${srcdir}/%.proto
	${PROTOC} $< --cpp_out=./ --proto_path=${srcdir} 2>&1 | sed -e "s,^\(.*\).proto,${srcdir}/\1.proto,"

%.pb.cc: %.pb
	@true

%.pb.h: %.pb
	@true

# protocol buffers. Need to be compiled first so the headers exist for the
# other modules to include. No need to list the headers, though, we don't
# distribute them and they're built alongside the cc files.
libprotobuf_a_SOURCES = \
    tColor.pb.cc \
    tCoord.pb.cc \
    tFunction.pb.cc \
    tPolynomial.pb.cc \
    nAuthentication.pb.cc \
    nNetwork.pb.cc \
    nNetObject.pb.cc \
    nNetObjectPrivate.pb.cc \
    nServerInfo.pb.cc \
    nConfig.pb.cc \
    eTeam.pb.cc \
    ePlayer.pb.cc \
    eNetGameObject.pb.cc \
    eVoter.pb.cc \
    eEventNotification.pb.cc \
    eLagCompensation.pb.cc \
    eTimer.pb.cc \
    zZone.pb.cc \
    zShape.pb.cc \
    gGame.pb.cc \
    gCycle.pb.cc \
    gWall.pb.cc \
    gZone.pb.cc \
    gWall.pb.cc \
    gAIBase.pb.cc 
#    .pb.cc \
#

# dependency for all automatically generated source files
all_protobuf_files: ${libprotobuf_a_SOURCES}

distclean-local:
	rm -f *.pb.cc *.pb.h

dist-hook:
#   copy .proto
	cp -R ${srcdir}/*.proto ${distdir}
#   and remove the .pb.cc files. The user needs protobuf dev files installed anyway,
#   so she has protoc available.
	rm -f ${distdir}/*.pb.cc

libprotobuf_a_CXXFLAGS  =$(AM_CXXFLAGS) -I@srcdir@ -I./
noinst_LIBRARIES = libprotobuf.a
