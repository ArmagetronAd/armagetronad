include $(top_srcdir)/Makefile.paths

# protocol buffer rules
%.pb: ${srcdir}/%.proto
	protoc $< --cpp_out=./ --proto_path=${srcdir} 2>&1 | sed -e "s,^\(.*\).proto,${srcdir}/\1.proto,"

%.pb.cc: %.pb
	@true

%.pb.h: %.pb
	@true

# protocol buffers. Need to be compiled first so the headers exist for the
# other modules to include.
libprotobuf_a_SOURCES = \
    tColor.pb.cc tColor.pb.h \
    tCoord.pb.cc tCoord.pb.h \
    tFunction.pb.cc tFunction.pb.h \
    tPolynomial.pb.cc tPolynomial.pb.h \
    nAuthentication.pb.cc nAuthentication.pb.h \
    nNetwork.pb.cc nNetwork.pb.h \
    nNetObject.pb.cc nNetObject.pb.h \
    nNetObjectPrivate.pb.cc nNetObjectPrivate.pb.h \
    nServerInfo.pb.cc nServerInfo.pb.h \
    nConfig.pb.cc nConfig.pb.h \
    eTeam.pb.cc eTeam.pb.h \
    ePlayer.pb.cc ePlayer.pb.h \
    eNetGameObject.pb.cc eNetGameObject.pb.h \
    eVoter.pb.cc eVoter.pb.h \
    eEventNotification.pb.cc eEventNotification.pb.h \
    eLagCompensation.pb.cc eLagCompensation.pb.h \
    eTimer.pb.cc eTimer.pb.h \
    zZone.pb.cc zZone.pb.h \
    zShape.pb.cc zShape.pb.h \
    gGame.pb.cc gGame.pb.h \
    gCycle.pb.cc gCycle.pb.h \
    gWall.pb.cc gWall.pb.h \
    gZone.pb.cc gZone.pb.h \
    gWall.pb.cc gWall.pb.h \
    gAIBase.pb.cc gAIBase.pb.h
#    .pb.cc .pb.h \
#

# dependency for all automatically generated source files
all_protobuf_files: ${libprotobuf_a_SOURCES}

distclean-local:
	rm -f ${libprotobuf_a_SOURCES}


libprotobuf_a_CXXFLAGS  =$(AM_CXXFLAGS) -I@srcdir@ -I./
noinst_LIBRARIES = libprotobuf.a
