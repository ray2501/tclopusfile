# -*- tcl -*-
# Tcl package index file, version 1.1
#
if {[package vsatisfies [package provide Tcl] 9.0-]} {
    package ifneeded opusfile 0.5 \
	    [list load [file join $dir libtcl9opusfile0.5.so] [string totitle opusfile]]
} else {
    package ifneeded opusfile 0.5 \
	    [list load [file join $dir libopusfile0.5.so] [string totitle opusfile]]
}
