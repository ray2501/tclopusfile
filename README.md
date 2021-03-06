tclopusfile
======

Tcl bindings for Opusfile library.

I use [Opusfile](https://www.opus-codec.org/docs/opusfile_api-0.6/index.html) library
to decode Opus audio file.


License
=====

MIT


Commands
=====

opusfile HANDLE path ?-buffersize size? ?-isurl boolean?  
HANDLE buffersize size  
HANDLE read   
HANDLE seek location  
HANDLE getTags  
HANDLE close


UNIX BUILD
=====

I only test tclopusfile under openSUSE LEAP 42.3 and Ubuntu 14.04.

Users need install Opusfile development files.
Below is an example for openSUSE:

    sudo zypper in opusfile-devel

Below is an example for Ubuntu:

    sudo apt-get install libopusfile-dev

Building under most UNIX systems is easy, just run the configure script
and then run make. For more information about the build process, see the
tcl/unix/README file in the Tcl src dist. The following minimal example
will install the extension in the /opt/tcl directory.

    $ cd tclopusfile
    $ ./configure --prefix=/opt/tcl
    $ make
    $ make install

If you need setup directory containing tcl configuration (tclConfig.sh),
below is an example:

    $ cd tclopusfile
    $ ./configure --with-tcl=/opt/activetcl/lib
    $ make
    $ make install

WINDOWS BUILD
=====

## MSYS2/MinGW-W64

Users can use MSYS2 pacman to install opusfile package.

	$ pacman -S mingw-w64-x86_64-opusfile

A not good news is you need modify `configure.ac` to indicate opusfile header files correct location.

    -    #TEA_ADD_INCLUDES([-I\"$(${CYGPATH} ${srcdir}/win)\"])
    +    TEA_ADD_INCLUDES([-I/mingw64/include/opus])

Execute `autoconf` to update configure. Then you can `./configure`, `make` and `make install`
to build and install.


Example
=====

Cowork with [tcllibao](https://github.com/ray2501/tcllibao).

    #
    # Using libao and Opusfile to play a opus file
    #

    package require libao
    package require opusfile

    if {$argc > 0} {
        set name [lindex $argv 0]
    } else {
        puts "Please input filename."
        exit
    }

    if {[catch {set data [opusfile file0 $name]}]} {
        puts "opusfile: read file failed."
        exit
    }
    set bits [dict get $data bits]

    # only for test seek function
    file0 seek 0

    libao::ao initialize
    set id [libao::ao default_id]

    libao::ao open_live $id -bits $bits \
        -rate [dict get $data samplerate] \
        -channels [dict get $data channels]

    while {[catch {set buffer [file0 read]}] == 0} {
        libao::ao play $buffer
    }

    file0 close
    libao::ao close
    libao::ao shutdown

Cowork with [tcllibao](https://github.com/ray2501/tcllibao) and
[TclX](https://github.com/flightaware/tclx).

    #
    # Using libao and Opusfile to play stream
    #

    package require libao
    package require opusfile
    package require Tclx

    if {$argc > 0} {
        set url [lindex $argv 0]
    } else {
        puts "Please input URL."
        exit
    }

    set ctrlc_flag 0

    proc mysig {} {
        global ctrlc_flag
        puts stdout "Aborting..."
        set ctrlc_flag 1
    }

    if {[catch {set data [opusfile url0 $url -isurl 1]}]} {
        puts "opusfile: open URL failed."
        exit
    }

    puts "Station Name: [dict get $data name]"
    puts "Station Description: [dict get $data description]"
    puts "Station Genre: [dict get $data genre]"
    puts "Station URL: [dict get $data url]"
    puts "Station Server: [dict get $data server]"

    set bits [dict get $data bits]

    libao::ao initialize
    set id [libao::ao default_id]

    set ctrlc_flag 0
    signal trap SIGINT mysig

    libao::ao open_live $id -bits $bits \
        -rate [dict get $data samplerate] \
        -channels [dict get $data channels]

    while {[catch {set buffer [url0 read]}] == 0} {
        libao::ao play $buffer
        if {$ctrlc_flag==1} {
            break;
        }
    }

    url0 close
    libao::ao close
    libao::ao shutdown

