# uninstaller name
uninstaller=@progname@@executable_suffix@-uninstall

# data locations
armadatadir=${aa_datadir}
texturedir=${armadatadir}
sounddir=${armadatadir}
languagedir=${armadatadir}
modeldir=${armadatadir}

# include $(top_srcdir)/Makefile.paths
include $(top_srcdir)/Makefile.manual

# what goes into the distribution
EXTRA_DIST = README-DEVELOPER README-KDevelop3_0 README-SDL \
armagetron.kdevelop armagetron.kdevelop.pcs armagetron.kdevses tron.ico \
models sound textures bootstrap.sh accustomdir.m4 fingerprint
# language/update.py

# resource is included as a subdir for the purpose of keeping this makefile short and delegating
# resource management to a more appropriate place anyway
SUBDIRS = src resource batch config desktop

# special tasks to do if we're building the master server
if BUILDMASTER
masterInstallTasks=	cd  $(DESTDIR)$(bindir); mv armagetronad_main_master$(EXEEXT) @prognamebase@-master$(executable_suffix)
masterUninstallTasks=rm -f $(DESTDIR)$(bindir)/@prognamebase@-master$(executable_suffix)
else
masterInstallTasks=echo 'No tasks for the master server'
masterUninstallTasks=echo 'No tasks for the master server'
endif

languagedata=language/english_base.txt language/english_base_notranslate.txt language/american.txt language/british.txt\
             language/deutsch.txt language/french.txt language/spanish.txt

nobase_dist_language_DATA=language/languages.txt.in $(languagedata) language/update.py

# What gets installed

if NEEDDATA
# install media data only when it is needed
nobase_texture_DATA=textures/cycle_body.png textures/cycle_wheel.png textures/dir_wall.png\
	textures/floor_a.png textures/floor_b.png textures/floor.png textures/font_extra.png\
	textures/font.png textures/font_s.png textures/icon.png textures/KGN_logo.png textures/rim_wall.png\
	textures/shadow.png textures/sky.png textures/title.jpg

nobase_sound_DATA=sound/cyclrun.wav sound/expl.wav

nobase_model_DATA=models/cycle_body.mod models/cycle_front.mod models/cycle_rear.mod
endif

nobase_language_DATA=language/languages.txt $(languagedata)

# generate configure.ac with version hardcoded and ChangeLog
dist-hook-scm:
#	replace dynamic version in configure.ac with the version this build is configured with
	sed -e "s/m4_include(version)//" -e "s/AUTOMATIC_VERSION/$(VERSION)/" < @srcdir@/configure.ac > $(distdir)/configure.ac

#   regenerate dependant files so they no longer reference to the file "version" which will be removed
	cp @srcdir@/bootstrap.sh $(distdir)
	cd $(distdir); rm version; sh ./bootstrap.sh
	rm -rf $(distdir)/autom4te.cache

dist-hook:
#   copy mac files
	cp -R ${srcdir}/MacOS ${distdir}

#	clear CVS
	find $(distdir) -name CVS -depth -exec rm -rf \{\} \;

#	clear SVN
	find $(distdir) -name .svn -depth -exec rm -rf \{\} \;

#   strip comments from translated files
	cd $(distdir)/language; python ./update.py --dist
#	restore timestamps
	cd $(distdir)/language; for f in *.txt; do test -r $$f.bak && touch -r $$f.bak $$f; done
	rm -f $(distdir)/language/*.bak

# 	generate ChangeLog and configure.ac
	test -r $(srcdir)/version && $(MAKE) dist-hook-scm || echo "Version not from SCM, skipping version fixing."

if USE_UNINSTALL

# installed location of the uninstall script
uninstall_location=$(DESTDIR)${bindir}/${uninstaller}

# generate and install uninstallation script
install-uninstall: @srcdir@/batch/make/uninstall Makefile
	$(install_sh_PROGRAM) $< $(uninstall_location)
	rm $(uninstall_location)
	MAKE="$(MAKE)" PREFIX="${prefix}" PROGTITLE="${progtitle}" SCRIPTDIR="${scriptdir}" sh $< $(uninstall_location) "$(external_uninstall_cmd)"
else
install-uninstall:
endif

# additional things on installation
install-exec-hook: install-uninstall
#	rename executable to armagetronad-VERSION and link to armagetronad
if BUILDMAIN
	cd  $(DESTDIR)${bindir}; mv armagetronad_main$(EXEEXT) @progname@$(executable_suffix)
endif

#	delegate to special tasks
	$(masterInstallTasks)

install-data-hook:
#	install systemwide files
if USE_SYSINSTALL
	DESTDIR=$(DESTDIR) ROOTDIR=$(ROOTDIR) bash $(DESTDIR)${scriptdir}/sysinstall install $(prefix)
endif

uninstall-hook:
#	delete executable and uninstaller
	rm -f $(DESTDIR)${bindir}/@progname@$(executable_suffix)

#	delegate to special tasks
	$(masterUninstallTasks)

if USE_UNINSTALL
	rm -f $(uninstall_location)
endif
#   rm -rf $(DESTDIR)$(aa_datadir)

# this has to be done first on uninstall/install. Unfortuantely.
# automake uses strict bottom-up logic here, so it is
# a special target recursively made from within src.
uninstall-first:
#	uninstall systemwide files
if USE_SYSINSTALL
	if test -z "$(AA_FAKE_UNINSTALL)"; then DESTDIR=$(DESTDIR) ROOTDIR=$(ROOTDIR) bash $(DESTDIR)${scriptdir}/sysinstall uninstall $(prefix); fi
endif

install-first:
#	uninstall previous version (if it exists), ignoring errors
	test -x $(uninstall_location) && $(uninstall_location) || true

distclean-local:
	rm -f uninstall.sh batch/relocate.in universal_variable* extrapaths src/tUniversalVariables.h* .changetag

# remove files left over by distcheck
distcheck-clean:
	chmod 755 -R @progname@-$(VERSION)*
	rm -rf @progname@-$(VERSION)*

