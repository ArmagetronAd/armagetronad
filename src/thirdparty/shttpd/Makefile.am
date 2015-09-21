noinst_LIBRARIES = libshttpd.a
libshttpd_a_SOURCES = shttpd.c shttpd.h
libshttpd_a_CFLAGS = -DMT -DEMBEDDED

# Compilation:
#    SSL support:      -DWITH_SSL -lssl -lcrypto
#    No CGI:           -DNO_CGI
#    No Authorization:     -DNO_AUTH
#    PUT,DELETE support:   -DWITH_PUT_AND_DELETE
#    Override max request size -DIO_MAX=xxxx
#    Default config file:  -DCONFIG=\"/etc/shttpd.conf\"
#    Typedef socklen_t     -DNO_SOCKLEN_T
#    Use embedded:     -DEMBEDDED
#    Use multithreading:   -DMT
#

