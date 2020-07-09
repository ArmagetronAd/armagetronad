# include $(top_srcdir)/Makefile.paths

# desktop files location
desktopfiledir=${aa_datadir}/desktop
protocolfiledir=${datadir}/services

# files for desktop integration
nobase_desktopfile_DATA=armagetronad-armagetronad.desktop armagetronad-armagetronad-handler.desktop \
	icons/16x16/armagetronad.png icons/32x32/armagetronad.png icons/48x48/armagetronad.png \
	armagetronad-link.xml armagetronad.appdata.xml
nobase_protocolfile_DATA=armagetronad.protocol

if OS_X_TOOLKIT
nobase_desktopfile_DATA += os-x/armagetronad.icns os-x/Info.plist
endif

EXTRA_DIST=icons/16x16/armagetronad.png\
           icons/32x32/armagetronad.png\
		   icons/48x48/armagetronad.png\
		   armagetronad-armagetronad.desktop.in \
		   armagetronad-armagetronad-handler.desktop.in\
		   armagetronad-link.xml.in\
		   armagetronad.protocol.in\
		   os-x/armagetronad.icns os-x/Info.plist.in\
   		   armagetronad.appdata.xml.in
