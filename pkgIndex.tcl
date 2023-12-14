# -*- tcl -*-
# Tcl package index file, version 1.1
#
if {[package vsatisfies [package provide Tcl] 9.0-]} {
    package ifneeded opusfile 0.4 \
	    [list load [file join $dir libtcl9opusfile0.4.so] [string totitle opusfile]]
} else {
    package ifneeded opusfile 0.4 \
	    [list load [file join $dir libopusfile0.4.so] [string totitle opusfile]]
}
