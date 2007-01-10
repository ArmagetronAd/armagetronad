swig_all = ${top_builddir}/src/swig/ext/armagetronad_wrap.cxx
swig_all_include = -I${srcdir}/.. -I${srcdir}/../tools -I${srcdir}/../network -I${srcdir}/../render -I${srcdir}/../ui -I${srcdir}/../tron -I${srcdir}/../engine

$(swig_all): ext/armagetronad.i ext/std/sstream.i ext/tools/tConfiguration.i  \
	     ext/tools/tDirectories.i ext/tools/tLocale.i ext/tools/tString.i \
	     ext/ui/uMenu.i
	mkdir -p `dirname ${@}`
	$(SWIG) -c++ -ruby -autorename $(swig_all_include) -o $@ $<

all-local: $(swig_all)
