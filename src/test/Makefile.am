# FIXME: don't duplicate this

libtools_a_CXXFLAGS  =$(AM_CXXFLAGS)  -I${top_srcdir}/src/tools
libnetwork_a_CXXFLAGS=$(libtools_a_CXXFLAGS)  -I${top_srcdir}/src/network
librender_a_CXXFLAGS =$(libtools_a_CXXFLAGS)  -I${top_srcdir}/src/render
libui_a_CXXFLAGS     =$(librender_a_CXXFLAGS) -I${top_srcdir}/src/ui
libengine_a_CXXFLAGS =$(libui_a_CXXFLAGS)     -I${top_srcdir}/src/network -I${top_srcdir}/src/engine 
libtron_a_CXXFLAGS   =$(libengine_a_CXXFLAGS) -I${top_srcdir}/src/thirdparty/particles -I${top_srcdir}/src/tron 

noinst_PROGRAMS = chat_prefix_test

chat_prefix_test_SOURCES = chat_prefix_test.cpp
chat_prefix_test_CXXFLAGS = $(libengine_a_CXXFLAGS)
chat_prefix_test_LDADD = ${top_builddir}/src/libenginecore.a ${top_builddir}/src/libengine.a \
                         ${top_builddir}/src/libnetwork.a ${top_builddir}/src/libui.a \
 						 ${top_builddir}/src/librender.a ${top_builddir}/src/libtools.a

