# uninstaller name
uninstaller=@progname@@executable_suffix@-uninstall

# data locations
armadatadir=${aa_datadir}
texturedir=${armadatadir}
sounddir=${armadatadir}
languagedir=${armadatadir}
modeldir=${armadatadir}
musicdir=${armadatadir}

# include $(top_srcdir)/Makefile.paths
include $(top_srcdir)/Makefile.manual

# what goes into the distribution
EXTRA_DIST = README-DEVELOPER README-Subversion README-SDL \
armagetron.kdevelop armagetron.kdevelop.pcs armagetron.kdevses tron.ico \
models sound textures bootstrap.sh music accustomdir.m4 fingerprint \
win32
# language/update.py

# resource is included as a subdir for the purpose of keeping this makefile short and delegating
# resource management to a more appropriate place anyway
SUBDIRS = src resource batch config desktop www-root

# special tasks to do if we're building the master server
if BUILDMASTER
masterInstallTasks=	cd  $(DESTDIR)$(bindir); mv armagetronad_main_master$(EXEEXT) @prognamebase@-master$(executable_suffix)
masterUninstallTasks=rm -f $(DESTDIR)$(bindir)/@prognamebase@-master$(executable_suffix)
else
masterInstallTasks=echo 'No tasks for the master server'
masterUninstallTasks=echo 'No tasks for the master server'
endif

languagedata=language/english_base.txt language/english_base_notranslate.txt language/american.txt language/british.txt\
             language/deutsch.txt language/french.txt language/spanish.txt language/polish_transliterated.txt language/polish.txt

nobase_dist_language_DATA=language/languages.txt.in $(languagedata) language/update.py

# What gets installed

if NEEDDATA
# install media data only when it is needed
nobase_texture_DATA=textures/cycle_body.png textures/cycle_wheel.png textures/dir_wall.png\
	textures/floor_a.png textures/floor_b.png textures/floor.png textures/font_extra.png\
	textures/font.png textures/font_s.png textures/icon.png textures/KGN_logo.png textures/rim_wall.png\
	textures/shadow.png textures/sky.png textures/title.jpg\
	textures/Armagetronad.ttf

nobase_sound_DATA=sound/1voicemale.ogg  sound/announcerGO.ogg  sound/expl.ogg\
	sound/2voicemale.ogg  sound/cycle_turn.ogg   sound/grind.ogg\
	sound/3voicemale.ogg  sound/cyclrun.ogg      sound/zone_spawn.ogg

nobase_music_DATA=music/titletrack.ogg  music/when.ogg music/doIknowyou.ogg\
	music/fortresswalk.ogg music/default.m3u music/fortresswalk.aatrack


nobase_model_DATA=models/cycle_body.mod models/cycle_front.mod models/cycle_rear.mod
endif

nobase_language_DATA=language/languages.txt $(languagedata)

doxy:
	cd src/doc && doxygen Doxyfile
	cd ../../

# generate configure.ac with version hardcoded and ChangeLog
dist-hook-scm:
#	replace dynamic version in configure.ac with the version this build is configured with
	sed -e "s/m4_include(version)//" -e "s/AUTOMATIC_VERSION/$(VERSION)/" < @srcdir@/configure.ac > $(distdir)/configure.ac

#	same for src/macosx/config_common.h.in
	sed -e "s/@ver""sion@/\"$(VERSION)\"/" < @srcdir@/src/macosx/config_common.h.in > $(distdir)/src/macosx/config_common.h.in

#   regenerate dependant files so they no longer reference to the file "version" which will be removed
	cp @srcdir@/bootstrap.sh $(distdir)
	cd $(distdir); rm -f aa_config.h.in version; sh ./bootstrap.sh; rm version
	rm -rf $(distdir)/autom4te.cache

dist-hook:
#   copy mac files
	cp -R ${srcdir}/MacOS ${distdir}

#	clear CVS
	find $(distdir) -depth -name CVS -exec rm -rf \{\} \;

#	clear SVN
	find $(distdir) -depth -name .svn -exec rm -rf \{\} \;

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
	ROOTDIR=$(ROOTDIR) DESTDIR=$(DESTDIR) MAKE="$(MAKE)" PREFIX="${prefix}" PROGTITLE="${progtitle}" PROGNAME="${progname}" SCRIPTDIR="${scriptdir}" sh $< $(uninstall_location) "$(external_uninstall_cmd)"
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
	test -n "$(uninstall_location)" && test -x "$(uninstall_location)" && "$(uninstall_location)" || true

distclean-local:
	rm -f uninstall.sh batch/relocate.in universal_variable* extrapaths src/tUniversalVariables.h* src/nTrueVersion.h* .changetag

# remove files left over by distcheck
distcheck-clean:
	chmod 755 -R @progname@-$(VERSION)*
	rm -rf @progname@-$(VERSION)*

