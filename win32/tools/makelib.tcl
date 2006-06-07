################################################################################
# makelib.tcl
#
# creates an MinGW conform lib*.a static library from a dll
# argument:
#    dll - path to the requested dll
# modified by Jochen Darley
# original: http://wiki.tcl.tk/2435
################################################################################

set dllextract_sed "-f[file join [file dirname [info script]] "dllextract.sed"]"
set dll [lindex $argv 0]
if {$dll == ""} {
    puts "creates an MinGW conform lib*.a static library from a dll"
    puts "usage: tclsh makedll.tcl <path/to/dll>"
    exit 0
}

set dllroot [file tail [file rootname $dll]]
set dlldef $dllroot.def
if {[regexp {^lib} $dllroot]} {
    set dlla $dllroot.a
} else {
    set dlla "lib$dllroot.a"
}

puts "writing $dlldef"
set res [exec objdump -p $dll | sed -n $dllextract_sed]
set fh [open $dlldef "w"]
puts $fh "EXPORTS"
puts $fh $res
close $fh

puts "creating $dlla"
exec dlltool --dllname $dll --input-def $dlldef --output-lib $dlla
puts "goodbye"
exit 0
