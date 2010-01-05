# all the SWIG interface files
INTERFACEFILES = ext/armagetronad.i ext/std/sstream.i ext/tools/tConfiguration.i  \
                 ext/tools/tCoord.i ext/tools/tColor.i ext/tools/tString.i ext/tools/tArray.i \
                 ext/engine/eTeam.i ext/engine/ePlayer.i			\
                 ext/tools/tDirectories.i ext/tools/tLocale.i			\ 
                 ext/ui/uMenu.i ext/engine/eGameObject.i ext/engine/eSensor.i	\
                 ext/tron/gSensor.i ext/defs.i ext/tron/gAIBase.i		\
                 ext/tron/gCycleMovement.i ext/tron/gCycle.i			\
                 ext/tron/gSpawn.i ext/tron/gArena.i				\
                 ext/engine/eNetGameObject.i ext/network/nNetObject.i

# although they should not be needed for a plain build, the interfaces are the true sources.
EXTRA_DIST = ${INTERFACEFILES}
#ext/armagetronad_wrap.cxx ext/armagetronad_wrap.h

if ENABLE_SCRIPTING
swig_all = ${top_builddir}/src/swig/ext/armagetronad_wrap.cxx
swig_all_include = -I${srcdir}/.. -I${srcdir}/../tools -I${srcdir}/../network -I${srcdir}/../render -I${srcdir}/../ui -I${srcdir}/../tron -I${srcdir}/../engine

if WITH_RUBY
$(swig_all): ${INTERFACEFILES}
	mkdir -p `dirname ${@}`
	$(SWIG) -c++ -ruby -autorename $(swig_all_include) -o $@ $<
endif

if WITH_PHP
$(swig_all): ${INTERFACEFILES}
	mkdir -p `dirname ${@}`
	$(SWIG) -c++ -php $(swig_all_include) -o $@ $<
endif

if WITH_LUA
$(swig_all): ${INTERFACEFILES}
	mkdir -p `dirname ${@}`
	$(SWIG) -c++ -lua $(swig_all_include) -o $@ $<
endif

if WITH_PYTHON
$(swig_all): ${INTERFACEFILES}
	mkdir -p `dirname ${@}`
	$(SWIG) -c++ -python $(swig_all_include) -o $@ $<
endif

all-local: $(swig_all)

endif

