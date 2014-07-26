Punix, Puny Unix operating system for TI-92+
============================================

by Christopher Williams

email: abbrev+punix@gmail.com

General information
-------------------

Punix is a Unix-like operating system for the TI-89 and TI-92+, but
primarily for the TI-92+. Its features will (or may) include:

* True preemptive multitasking
* Audio output via the link port
* Data I/O via the link port (which will allow a user program to
  send/receive variables to and from other types of calculators)
* 60x20 virtual terminals
* Filesystem on the FlashROM
* Floating-point emulation
* And more!

The Punix kernel is a standard monolithic kernel design, similar to the
BSD's and clones. I'm using a lot of code from 2.11BSD, 4.4BSD-Lite,
V6/V7 UNIX, Linux, UZIX, and Minix (in that order of amount of code, it
seems), and I'm structuring the source tree somewhat similar to BSD, so
if you understand that you probably can understand my kernel.

The kernel is nearly complete as of this writing. Preemptive scheduling
works fairly well, and the terminal is usable. Processes can spawn new
processes using the vfork() and execve() system calls. The file system
is the last major feature that is still largely incomplete. Because of
the missing file system, execve() currently understands a few hard-coded
command names (eg, "sh" and "top"). Work is underway to develop the file
system and remove this final obstacle.

Building Punix
--------------

To build Punix you will need GCC4TI (last I checked, TIGCC has unfixed
bugs in the official release which renders Punix unusable). Also, a
Unix-like environment is preferred because of the use of makefiles and
the Netpbm image utilities. You may have luck using MSYS or Cygwin, but
these are not supported.

To build Punix, go to the Punix root directory (the one that contains
the src and tools directories, among others) in a shell and run make.
This will build the kernel for all supported platforms (currently TI-92+
and TI-89) and the libraries. Once built, the kernels will be named
punix-89.tib and punix-9x.tib. These files can be used in an emulator
like TiEmu.

Issues
------

You will likely encounter problems while trying to build Punix. If you
can't solve the problem yourself, feel free to contact me, including as
much detail about the problem as you can. It may be an issue I'm
familiar with. Good luck!

Roadmap
-------

Here is a general roadmap of work to be done on Punix.

* Complete the file system in the kernel
* Complete the floating-point emulator in the kernel (a non-essential
  feature)
* Port Punix to the TI-89Ti (and add other features like USB)
* Build a userland development environment (ie, cross compiler and
  tools)
* Port libraries, tools, applications, and games to Punix

