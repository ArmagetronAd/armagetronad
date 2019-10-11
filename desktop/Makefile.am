# include $(top_srcdir)/Makefile.paths

# desktop files location
desktopfiledir=${aa_datadir}/desktop
protocolfiledir=${datadir}/services

# files for desktop integration
nobase_desktopfile_DATA=armagetronad-armagetronad.desktop icons/small/armagetronad.png icons/medium/armagetronad.png icons/large/armagetronad.png armagetronad-armagetronad-handler.desktop armagetronad-link.xml
nobase_protocolfile_DATA=armagetronad.protocol

if OS_X_TOOLKIT
nobase_desktopfile_DATA += os-x/armagetronad.icns os-x/Info.plist
endif

EXTRA_DIST=icons/small/armagetronad.png\
           icons/medium/armagetronad.png\
		   icons/large/armagetronad.png\
		   armagetronad-armagetronad.desktop.in \
		   armagetronad-armagetronad-handler.desktop.in\
		   armagetronad-link.xml.in\
		   armagetronad.protocol.in\
		   os-x/armagetronad.icns os-x/Info.plist.in
