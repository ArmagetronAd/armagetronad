if BUILDFAKERELEASE
SUBDIRS=
else
if BUILDDEDICATED
SUBDIRS=mathexpr
else
if BUILDREGULAR
SUBDIRS=particles mathexpr
if !OS_X_TOOLKIT
	SUBDIRS += scrap
endif
endif
endif
endif
