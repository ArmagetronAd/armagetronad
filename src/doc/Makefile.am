include HtmlMakefile

SUBDIRS = net

# almost all possilbe automatically generated HTML files should be considered documentation
doc_html_files=$(filter-out navbar.html head.html, $(auto_html_files))

doc_txt_files=commands.txt COPYING.txt Content_Creation/Howto-Maps.txt
docfiles=$(doc_txt_files) $(doc_html_files)

COPYING.txt:
	cp ${top_srcdir}/COPYING $@

# autogenerate commands.txt from executable
if BUILDFAKERELEASE
commands.html.post: commands.html
	if test "`grep 'Running FAKE' $(srcdir)/commands.txt`" != ""; then touch -t 198001010000 $(srcdir)/commands.txt; fi

all-local: commands.html.post
endif

$(srcdir)/commands.txt: ${top_srcdir}/language/*
	rm -f $@
	cd $(top_builddir)/src && $(MAKE) armagetronad_main$(EXEEXT) && cd .. && ./src/armagetronad_main$(EXEEXT) --vardir ./rubbish --doc 2>&1 | sed -e "s,^\[0\] ,," | grep -v "_help$$" | grep -v "^changed" | grep -v "^Executable path: " | grep -v "^TestPath" | grep -v "^Testing data path: " | grep -v "^Testing configuration path: " | grep -v "^Closing socket" | grep -v "^Locale item" > src/doc/commands.txt || true
	test -r commands.txt || echo "Sorry, command help generation failed." > commands.txt
	test -r $@ || mv commands.txt $@

print:
	echo $(auto_html_files_m4_dist)

# install generated html files and put them directly into the distribution
textdir=$(aa_datadir)/doc
text_DATA=$(doc_txt_files)

htmldir=$(aa_datadir)/doc
html_DATA=$(doc_html_files)
distclean-local:
	rm -f COPYING.txt

dist-hook: htmldist-hook ${srcdir}/commands.txt ${docfiles}
	cp ${srcdir}/commands.txt $(distdir)
	cp -R ${srcdir}/Content_Creation $(distdir)
	cp -R ${doc_html_files} $(distdir)

# make target for testing documentation without installing
localtest: all Howto-Maps.txt ${docfiles}

Howto-Maps.txt:
	ln -s ${srcdir}/Content_Creation/$@ $@
