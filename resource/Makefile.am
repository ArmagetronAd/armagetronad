# include $(top_srcdir)/Makefile.paths

# installation destination
resourcedir=${aa_datadir}/resource

sourcefiles = $(shell test -d ${srcdir}/proto && find ${srcdir}/proto -type f | grep -v CVS | grep -v "~")

included: ${sourcefiles} ${top_srcdir}/batch/make/sortresources.py ${top_srcdir}/batch/make/copyresources.py Makefile
	python ${top_srcdir}/batch/make/copyresources.py ${srcdir}/proto $@
# @sh ${top_srcdir}/batch/make/sortresources ${srcdir}/proto $@ ${top_srcdir}/batch/make/sortresources.py || { rm -rf $@; false; }

all: included

linked_included.%: included
	rm -rf $@
	if test ! -r $@; then \
		if test -d included; then\
			ln -sf ./included $@;\
		else\
			ln -sf ${srcdir}/included $@;\
		fi;\
	fi

install-data-local: linked_included.install
#   copy processed included resources into target directory
	mkdir -p ${DESTDIR}${resourcedir}/included
	cp -r $</* ${DESTDIR}${resourcedir}/included
	find ${DESTDIR}${resourcedir} -type d -exec chmod 755 \{\} \;
	find ${DESTDIR}${resourcedir} -type f -exec chmod 644 \{\} \;
	rm -f $<	

uninstall-local:
#   remove resources
	rm -rf ${DESTDIR}${resourcedir}/included

dist-hook: linked_included.dist
#   copy processed included resources into distribution
	mkdir -p $(distdir)/included
	cp -r $</* $(distdir)/included/

#	delete temporary files
	rm -f $<

#	update included resources to correct dtd version
	for type in `ls included | grep '.dtd$$' | grep -v -- - | sed -e 's,.dtd,,'`; do\
	    echo fixing referenced DTD version of XML files of type $${type}...; \
		if test -r included/.$${type}version; then \
			for file in `find $(distdir)/included/ -name "*.aa$${type}.xml" -o -name "*.$${type}.xml"`; do \
				echo "Adapting dtd in $$file" | sed -e "s,$(distdir)/included/,,";\
				sed -e "s,$${type}.dtd,AATeam/$${type}-`cat included/.$${type}version`.dtd,g" < $$file > $$file.processed ;\
				mv $$file.processed $$file ;\
			done ;\
		fi; \
		rm -f $(distdir)/included/$${type}.dtd; \
	done

clean-local:
	test -d ${srcdir}/proto && rm -rf included || true
