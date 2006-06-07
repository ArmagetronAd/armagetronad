# exports.sed - Copyright (C) 2001 Pat Thoyts <Pat.Thoyts@bigfoot.com>
#
# Build an exports list from a Windows DLL.
#
/[ 	]*ordinal hint/,/^[ 	]*Summary/{
 /^[ 	]\+[0123456789]\+/{
   s/^[ 	]\+[0123456789]\+[ \t]\+[0123456789ABCDEFabcdef]\+[ 	]\+[0123456789ABCDEFabcdef]\+[ 	]\+\(.*\)/\1/p
 }
}
