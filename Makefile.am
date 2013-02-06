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
win32 major_version
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
             language/deutsch.txt language/french.txt language/spanish.txt language/polish.txt language/russian.txt

nobase_dist_language_DATA=language/languages.txt.in $(languagedata) language/update.py

# What gets installed

if NEEDDATA
# install media data only when it is needed
nobase_texture_DATA=textures/cycle_body.png textures/cycle_wheel.png textures/dir_wall.png\
	textures/floor_a.png textures/floor_b.png textures/floor.png textures/font_extra.png\
	textures/font.png textures/font_s.png textures/icon.png textures/rim_wall.png\
	textures/shadow.png textures/sky.png textures/title.jpg\
	textures/settings_textures.cfg \
	textures/message_background.png \
	textures/tutorials/teamstart/success.png \
	textures/tutorials/teamstart/background.png \
	textures/tutorials/teamstart/straight.png \
	textures/tutorials/doublegrind/success.png \
	textures/tutorials/doublegrind/toohard1.png \
	textures/tutorials/doublegrind/background.png \
	textures/tutorials/doublegrind/toohard2.png \
	textures/tutorials/doublegrind/toohard3.png \
	textures/tutorials/grinding/fail2.png \
	textures/tutorials/grinding/nogrind.png \
	textures/tutorials/grinding/fail3.png \
	textures/tutorials/grinding/fail1.png \
	textures/tutorials/grinding/grind.png \
	textures/tutorials/survival/fail2.png \
	textures/tutorials/survival/fail3.png \
	textures/tutorials/survival/spiral.png \
	textures/tutorials/survival/fail1.png \
	textures/tutorials/survival/wiggle.png \
	textures/tutorials/navigation/fail2.png \
	textures/tutorials/navigation/fancy.png \
	textures/tutorials/navigation/fail3.png \
	textures/tutorials/navigation/indirect.png \
	textures/tutorials/navigation/fail1.png \
	textures/tutorials/navigation/zone.png \
	textures/tutorials/navigation/direct.png \
	textures/tutorials/navigation/start.png \
	textures/tutorials/clipart/fail.png \
	textures/tutorials/clipart/background.png \
	textures/tutorials/clipart/check.png \
	textures/tutorials/clipart/circle.png \
	textures/tutorials/speedkill/adjust3.png \
	textures/tutorials/speedkill/adjust1.png \
	textures/tutorials/speedkill/adjust2.png \
	textures/tutorials/speedkill/success2.png \
	textures/tutorials/speedkill/success1.png \
	textures/tutorials/speedkill/fail_donothing.png \
	textures/tutorials/speedkill/success3.png \
	textures/tutorials/speedkilldefense/fail2.png \
	textures/tutorials/speedkilldefense/fail3.png \
	textures/tutorials/speedkilldefense/trap3.png \
	textures/tutorials/speedkilldefense/fail1.png \
	textures/tutorials/speedkilldefense/trap1.png \
	textures/tutorials/speedkilldefense/trap2.png \
	textures/tutorials/speedkilldefense/start.png \
	textures/tutorials/conquest/success.png \
	textures/tutorials/conquest/start.png \
	textures/tutorials/teamstart/teamstart.txt \
	textures/tutorials/doublegrind/doublegrind.txt \
	textures/tutorials/grinding/grinding.txt \
	textures/tutorials/survival/survival.txt \
	textures/tutorials/navigation/navigation.txt \
	textures/tutorials/speedkill/speedkill.txt \
	textures/tutorials/speedkilldefense/speedkilldefense.txt \
	textures/tutorials/conquest/conquest.txt \
	textures/Armagetronad.ttf

nobase_sound_DATA=sound/1voicemale.ogg  sound/announcerGO.ogg  sound/expl.ogg\
	sound/2voicemale.ogg  sound/cycle_turn.ogg   sound/grind.ogg\
	sound/3voicemale.ogg  sound/cyclrun.ogg      sound/zone_spawn.ogg\
    sound/cyclrun.wav

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
#	replace dynamic version in configure.ac with the version this build is configured with,
#   as well as default package and program name
	sed \
	-e "s/m4_include(version)//" \
	-e "s/AC_INIT(armagetronad,AUTOMATIC_VERSION)/AC_INIT(@prognamebase@,$(VERSION))/" \
	-e "s/  progname=.*/  progname='@prognamebase@'/" \
	-e "s/  progtitle=.*/  progtitle='@progtitle@'/" \
	< @srcdir@/configure.ac > $(distdir)/configure.ac

#   rebrand Windows config directories
	sed	-e "s,/Armagetron,/@progtitle@," \
	< @srcdir@/src/win32/aa_config.h > $(distdir)/src/win32/aa_config.h
	sed	-e "s,/ArmagetronDedicated,/@progtitle@@progtitle_server@," \
	< @srcdir@/src/win32_ded/aa_config.h > $(distdir)/src/win32_ded/aa_config.h

#   rebrand OSX config directories
	sed	-e "s,/Armagetron Advanced,/@progtitle@," \
	< @srcdir@/src/macosx/client/aa_config.h > $(distdir)/src/macosx/client/aa_config.h
	sed	-e "s,/Armagetron Advanced Dedicated,/@progtitle@@progtitle_server@," \
	< @srcdir@/src/macosx/dedicated/aa_config.h > $(distdir)/src/macosx/dedicated/aa_config.h

#   regenerate dependant files so they no longer reference to the file "version" which will be removed
	cp @srcdir@/bootstrap.sh $(distdir)
	cd $(distdir); rm -f aa_config.h.in version; sh ./bootstrap.sh
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

#   fix internal version info
	mv $(distdir)/src/tTrueVersion.h $(distdir)/src/tFakeVersion.h

#   generate yaml version info
	echo "---" > $(distdir)/version.yaml
	sed -e 's,^.*TRUE_ARMAGETRONAD_,,' -e 's, \+,: ,' -e 's, *\([0-9a-Z]\+\)$$, "\1",' < $(distdir)/src/tFakeVersion.h >> $(distdir)/version.yaml

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
	cd  "$(DESTDIR)${bindir}"; mv armagetronad_main$(EXEEXT) @progname@$(executable_suffix)
endif
if BUILDDEDICATED
	cd  "$(DESTDIR)${bindir}"; mv armagetronad_serverquery$(EXEEXT) @prognamebase@-serverquery$(executable_suffix)
endif

#	delegate to special tasks
	$(masterInstallTasks)

install-data-hook:
#	install systemwide files
if USE_SYSINSTALL
	DESTDIR=$(DESTDIR) ROOTDIR=$(ROOTDIR) bash "$(DESTDIR)${scriptdir}/sysinstall" install "$(prefix)"
endif
if OS_X_TOOLKIT
	${top_srcdir}/batch/make/symlink-relative "$(DESTDIR)${prefix}" --files-in-directory "$(DESTDIR)${aa_datadir}" --file-with-name "$(DESTDIR)${aa_sysconfdir}" config
#   TODO use progtitle (requires substituting variables in Info.plist)
	${top_srcdir}/batch/make/symlink-relative "$(DESTDIR)${prefix}/../MacOS" --file-with-name "$(DESTDIR)${bindir}/@progname@${executable_suffix}" "Armagetron Advanced"

	rm -rf "$(DESTDIR)${prefix}/../Frameworks"
	cp -R "${ARMAGETRONAD_OS_X_TOOLKIT}/Frameworks" "$(DESTDIR)${prefix}/.."
	
	install_name_tool -delete_rpath "${ARMAGETRONAD_OS_X_TOOLKIT}/Frameworks" "$(DESTDIR)${prefix}/../MacOS/Armagetron Advanced" > /dev/null 2>&1 || true
	install_name_tool -add_rpath "@executable_path/../../Frameworks" "$(DESTDIR)${prefix}/../MacOS/Armagetron Advanced"
endif

uninstall-hook:
#	delete executable and uninstaller
	rm -f $(DESTDIR)${bindir}/@progname@$(executable_suffix)
if BUILDDEDICATED
	rm -f $(DESTDIR)${bindir}/@prognamebase@-serverquery$(executable_suffix)
endif

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
	rm -f uninstall.sh batch/relocate.in universal_variable* extrapaths src/tUniversalVariables.h* src/tTrueVersion.h* .changetag

# remove files left over by distcheck
distcheck-clean:
	chmod 755 -R @progname@-$(VERSION)*
	rm -rf @progname@-$(VERSION)*

