# This Makefile does not list all the dependencies of each object file, so
# usually it is best to 'make clean' before making the TIB file.

MAKE	= gmake
CALC	= TI92P

PUNIXBASE	= ../../..

STDINC	= ${PUNIXBASE}/include
SYSINC	= ${PUNIXBASE}/src/sys/h
DATE := $(shell date +'%Y-%m-%d %H:%M:%S%z')
REALTIME := $(shell date -d "$(DATE)" +%s)
STUPIDDATE := $(shell date -d "$(DATE)" +%m/%d/%Y)
UNAME_SYSNAME := Punix
UNAME_NODENAME := timmy
UNAME_RELEASE := 0.08
UNAME_VERSION := $(DATE)
UNAME_MACHINE := m68k



CC	= ${TIGCC}/bin/tigcc
CFLAGS	= -O2 -fomit-frame-pointer -Wall -nostdinc -fno-builtin -mno-bss \
	-I${SYSINC} -I. -I- -I${STDINC} -D__KERNEL__ -D${CALC}= \
	-DSMALLGLYPHS= -DREALTIME=${REALTIME}UL

AS	= ${TIGCC}/bin/tigcc
ASFLAGS	= -c -Wa,-m68881,--warn,-I${SYSINC},--defsym,${CALC}= #-l

LD	= ${TIGCC}/bin/tigcc
LDFLAGS	= --outputbin --flash-os #--optimize-branches --optimize-relocs \
#          --omit-bss-init

# core
OBJ	=  start.o vectors.o main.o entry.o version.o calc.o signal.o
# syscalls
OBJ	+= syscall.o sysent.o sys_desc.o sys_proc.o sys_prot.o sys_time.o
OBJ	+= pipe.o sys_sig.o
# devices
OBJ	+= dev.o dev_audio.o dev_flash.o dev_link.o dev_misc.o dev_vt.o
OBJ	+= lcd.o dev_tty.o keyscan.o batt.o
# helper routines
OBJ	+= kputs.o scroll.o
OBJ	+= copy.o fio.o
OBJ	+= setjmp.o m68k.o string.o trap.o queue.o flash.o bio.o # rdwri.o
OBJ	+= context.o
OBJ	+= long.o subr.o tty.o inode.o process.o pfs_alloc.o pfs_bmap.o
OBJ	+= panic.o abort.o assert.o kprintf.o printf.o clock.o #globals.o
OBJ	+= ffs.o loadav.o drawglyph.o heap.o sched.o bogomips.o
OBJ	+= fpuemu.o sysctl.o
OBJ	+= userstart.o usermain.o qsort.o

#OBJ = testvectors.o

TMPUNAME = uname.sysname uname.nodename uname.release uname.version uname.machine stupiddate

all:	punix-9x.tib
scratch: clean all

punix-9x.tib:	${OBJ}
	${LD} ${LDFLAGS} -o punix $^ #${TIGCC}/lib/flashos.a #${TIGCC}/lib/tigcc.a

obj:	${OBJ}

${OBJ}: ${SYSINC}/globals.h ${SYSINC}/punix.h ${SYSINC}/param.h
${TMPUNAME}: Makefile

########################################
# source dependencies
########################################

heap.o: heap.c
	${CC} ${CFLAGS} $< -c -o $@
start.o: start.s
vectors.o: vectors.s
version.o: version.s ${TMPUNAME}
uname.sysname:
	echo -n "$(UNAME_SYSNAME)" >$@
uname.nodename:
	echo -n "$(UNAME_NODENAME)" >$@
uname.release:
	echo -n "$(UNAME_RELEASE)" >$@
uname.version:
	echo -n "$(UNAME_VERSION)" >$@
uname.machine:
	echo -n "$(UNAME_MACHINE)" >$@
stupiddate:
	echo -n "$(STUPIDDATE)" >$@
entry.o: entry.s ${SYSINC}/signal.inc
drawcell.o: drawcell.s ${SYSINC}/lcd.inc
calc.o: calc.s
#switchthread.o:
m68k.o: m68k.s
string.o: string.s
setjmp.o: setjmp.s
context.o: context.s
#${SYSINC}/lcd.inc: ${SYSINC}/lcdglo.inc
#${SYSINC}/lcd.h: ${SYSINC}/lcdglo.h
#${SYSINC}/cell.h: ${SYSINC}/cellglo.h ${SYSINC}/lcd.h
#${SYSINC}/cell.inc: ${SYSINC}/cellglo.inc ${SYSINC}/lcd.inc
${SYSINC}/proc.h: ${SYSINC}/file.h ${STDINC}/sys/types.h \
	${STDINC}/time.h ${STDINC}/signal.h
${SYSINC}/process.h: ${SYSINC}/proc.h
dev_vt.o: dev_vt.c glyphsets/all
keyscan.o: keyscan.c ${SYSINC}/kbd.h glyphsets/all

md5.o: md5.c
qsort.o: qsort.c
unpack.o: unpack.c
kputs.o: kputs.c
kputchar.o: kputchar.c ${SYSINC}/cell.h ${SYSINC}/scrollcell.h ${SYSINC}/drawcell.h
syscall.o: syscall.c ${SYSINC}/sysent.h ${SYSINC}/process.h
sysent.o: sysent.c ${SYSINC}/sysent.h
main.o: main.c ${SYSINC}/proc.h ${STDINC}/assert.h
sys_desc.o: sys_desc.c ${SYSINC}/proc.h ${SYSINC}/punix.h ${SYSINC}/file.h \
		${SYSINC}/process.h
sys_proc.o: sys_proc.c ${SYSINC}/proc.h ${SYSINC}/punix.h ${SYSINC}/process.h 
sys_prot.o: sys_prot.c
sys_time.o: sys_time.c
panic.o: panic.c ${SYSINC}/punix.h
process.o: process.c ${SYSINC}/proc.h ${SYSINC}/process.h \
		${STDINC}/assert.h
hanoi.o: hanoi.c
abort.o: abort.c ${SYSINC}/punix.h
assert.o: assert.c
kprintf.o: kprintf.c
fio.o: fio.c
#rdwri.o: rdwri.c
#bio.o: bio.c
dev.o: dev.c
#bmap.o: bmap.c
trap.o: trap.c
#queue.o: queue.c
queue.o: queue.s
dev_link.o: dev_link.c
dev_audio.o: dev_audio.c
dev_flash.o: dev_flash.c

#${SYSINC}/cellglo.h ${SYSINC}/cellglo.inc ${SYSINC}/glo.h ${SYSINC}/glo.inc \
${SYSINC}/lcdglo.h ${SYSINC}/lcdglo.inc ${SYSINC}/procglo.h ${SYSINC}/procglo.inc \
${SYSINC}/vectorsglo.h ${SYSINC}/vectorsglo.inc ${SYSINC}/heapglo.h ${SYSINC}/heapglo.inc: \
${SYSINC}/cellglo.def ${SYSINC}/glo.def ${SYSINC}/lcdglo.def \
${SYSINC}/procglo.def ${SYSINC}/vectorsglo.def ${SYSINC}/heapglo.def
#	${MAKE} -C ${SYSINC}

########################################
# Data (images, glyphs)
########################################

glyphsets/all:
	${MAKE} -C glyphsets all

clean:
	rm -f *.bin *.o punix-9x.tib ${TMPUNAME}
	${MAKE} -C glyphsets clean
	#${MAKE} -C ${SYSINC} clean
