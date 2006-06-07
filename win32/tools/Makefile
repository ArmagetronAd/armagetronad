# Makefile - Copyright (C) 2001 Pat Thoyts <Pat.Thoyts@bigfoot.com>
#
# How to create a gcc compatible linker library for a DLL from an MSVC
# produced .LIB file or DLL. If you want to use the .LIB files, swap over
# the commented sections and build the .DEF file using the other method.

DLLTOOL=dlltool --kill-at

all: libtcl83.a libtk83.a

#-------------------------------------------------------------------------

tcl83.def:
	echo EXPORTS > $@
	dumpbin /exports ..\bin\tcl83.dll | sed -nf exports.sed >> $@
#tcl83.def:
#	echo EXPORTS > $@
#	nm tcl83.lib | sed -n "/00000000 T/s/\([^_]*_\)//p" >> $@

#tcl83.def:
#	echo EXPORTS > $@
#      objdump -p ..\bin\tcl83.dll | sed -nf dllextract.sed >> $@

libtcl83.a: tcl83.def
	$(DLLTOOL) --dllname ../bin/tcl83.dll --input-def $< --output-lib $@

#-------------------------------------------------------------------------

tk83.def:
	echo EXPORTS > $@
	dumpbin /exports ..\bin\tk83.dll | sed -nf exports.sed >> $@
#tk83.def:
#	echo EXPORTS > $@
#	nm tk83.lib | sed -n "/00000000 T/s/\([^_]*_\)//p" >> $@

#tk83.def:
#	echo EXPORTS > $@
#      objdump -p ..\bin\tk83.dll | sed -nf dllextract.sed >> $@

libtk83.a: tk83.def
	$(DLLTOOL) --dllname ../bin/tk83.dll --input-def $< --output-lib $@

#-------------------------------------------------------------------------

clean:
	@del tcl83.def
	@del tk83.def

realclean: clean
	@del libtcl83.a
	@del libtk83.a

.PHONY: clean realclean

#
# Local variables:
#   mode: makefile
# End:
#