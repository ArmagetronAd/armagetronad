swig_opts = -c++ -ruby -autorename

swig_tools = ${top_builddir}/src/swig/ext/armagetronad/tools/tools_wrap.cxx
swig_tools_include = -I${srcdir}/.. -I${srcdir}/../tools -I${srcdir}/ext/armagetronad/tools

swig_network = ${top_builddir}/src/swig/ext/armagetronad/network/network_wrap.cxx
swig_network_include = $(swig_tools_include) -I${srcdir}/../network -I${srcdir}/ext/armagetronad/network

swig_render = ${top_builddir}/src/swig/ext/armagetronad/render/render_wrap.cxx
swig_render_include = $(swig_network_include) -I${srcdir}/../render -I${srcdir}/ext/armagetronad/render

swig_ui = ${top_builddir}/src/swig/ext/armagetronad/ui/ui_wrap.cxx
swig_ui_include = $(swig_render_include) -I${srcdir}/../ui -I${srcdir}/ext/armagetronad/ui

swig_engine = ${top_builddir}/src/swig/ext/armagetronad/engine/engine_wrap.cxx
swig_engine_include = $(swig_ui_include) -I${srcdir}/../engine -I${srcdir}/ext/armagetronad/engine

swig_tron = ${top_builddir}/src/swig/ext/armagetronad/tron/tron_wrap.cxx
swig_tron_include = $(swig_engine_include) -I${srcdir}/../tron -I${srcdir}/ext/armagetronad/tron

$(swig_tools): ext/armagetronad/tools/tools.i ext/armagetronad/tools/tArray.i             \
	       ext/armagetronad/tools/tColor.i ext/armagetronad/tools/tCommandLine.i      \
               ext/armagetronad/tools/tConfiguration.i	ext/armagetronad/tools/tCoord.i   \
               ext/armagetronad/tools/tDirectories.i ext/armagetronad/tools/tError.i      \
       	       ext/armagetronad/tools/tEventQueue.i ext/armagetronad/tools/tLinkedList.i  \
       	       ext/armagetronad/tools/tLocale.i ext/armagetronad/tools/tMemManager.i      \
       	       ext/armagetronad/tools/tRandom.i ext/armagetronad/tools/tResourceManager.i \
       	       ext/armagetronad/tools/tRing.i ext/armagetronad/tools/tString.i            \
       	       ext/armagetronad/tools/tSysTime.i ext/armagetronad/tools/tTypemaps.i       \
	       ext/armagetronad/tools/tCallback.i
	
	$(SWIG) $(swig_opts) $(swig_tools_include) -o $@ $<

$(swig_network): ext/armagetronad/network/network.i ext/armagetronad/network/nConfig.i \
                 ext/armagetronad/network/nNetwork.i ext/armagetronad/network/nTypemaps.i

	$(SWIG) $(swig_opts) $(swig_network_include) -o $@ $<

$(swig_render): ext/armagetronad/render/render.i ext/armagetronad/render/rTypemaps.i ext/armagetronad/render/rScreen.i
	$(SWIG) $(swig_opts) $(swig_render_include) -o $@ $<

$(swig_ui): ext/armagetronad/ui/ui.i ext/armagetronad/ui/ui.i \
            ext/armagetronad/ui/uMenu.i ext/armagetronad/ui/uTypemaps.i

	$(SWIG) $(swig_opts) $(swig_ui_include) -o $@ $<

$(swig_engine): ext/armagetronad/engine/engine.i ext/armagetronad/engine/eAxis.i           \
                ext/armagetronad/engine/eLagCompensation.i ext/armagetronad/engine/ePath.i \
                ext/armagetronad/engine/ePlayer.i ext/armagetronad/engine/eRectangle.i     \
                ext/armagetronad/engine/eTypemaps.i ext/armagetronad/engine/eVoter.i

	$(SWIG) $(swig_opts) $(swig_engine_include) -o $@ $<

$(swig_tron): ext/armagetronad/tron/tron.i ext/armagetronad/tron/gTypemaps.i ext/armagetronad/tron/gGame.i \
	      ext/armagetronad/tron/gRotation.i
	$(SWIG) $(swig_opts) $(swig_tron_include) -o $@ $<

all-local: $(swig_tools) $(swig_network) $(swig_render) $(swig_ui) $(swig_engine) $(swig_tron)
	rm -rf ${top_builddir}/src/swig/setup.rb
	rm -rf ${top_builddir}/src/swig/lib
	cp -r ${srcdir}/setup.rb ${top_builddir}/src/swig
	cp -r ${srcdir}/lib ${top_builddir}/src/swig
	cd ${top_builddir}/src/swig && $(RUBY) setup.rb config && $(RUBY) setup.rb setup

install-exec-local:
	cd ${top_builddir}/src/swig && $(RUBY) setup.rb install
