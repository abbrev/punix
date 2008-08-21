Punix, Puny Unix operating system for TI-92+
by Christopher Williams
email: abbrev@gmail.com

The Punix kernel is a standard monolithic kernel design, similar to the BSD's and clones. I'm using a lot of code from 2.11BSD, 4.4BSD-Lite, V6 UNIX, Linux, UZIX, and Minix (in that order of amount of code, it seems), and I'm structuring the source tree somewhat similar to BSD, so if you understand that you probably can understand my kernel.

Currently the kernel is very incomplete. It at least boots, showing only the boot-up screen (with copyright and license information) and an error from the init code complaining that it can't execv /etc/init. This is because the kernel does not yet have a filesystem, so the execv system call simply cannot work yet.

Preemptive multi-tasking works, for the most part. There is not much concept of priority levels; the scheduler is a simple round-robin type. This is no problem for now but will probably prove to be a performance bottleneck later on.
