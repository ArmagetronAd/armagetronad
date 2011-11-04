include ../HtmlMakefile

# almost all possilbe automatically generated HTML files should be considered documentation
doc_html_files=$(filter-out navbar.html head.html, $(auto_html_files))

doc_txt_files=
docfiles=$(doc_txt_files) $(doc_html_files)

# install generated html files and put them directly into the distribution
htmldir=$(aa_datadir)/doc/net
html_DATA=$(docfiles)

dist-hook: htmldist-hook ${doc_html_files}
	cp -R ${doc_html_files} $(distdir)

