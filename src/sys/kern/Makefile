# This Makefile does not list all the dependencies of each object file, so
# usually it is best to 'make clean' before making the TIB file.

CALC	= TI92P

PUNIXBASE	= ../../..

STDINC	= ${PUNIXBASE}/include
INCLUDE	= ${PUNIXBASE}/src/sys
SYS	= ${INCLUDE}/sys

CC	= ${TIGCC}/bin/tigcc
CFLAGS	= -O2 -fomit-frame-pointer -Wall -nostdinc -fno-builtin -mno-bss \
	-I${SYS} -I${STDINC} -D__KERNEL__

AS	= ${TIGCC}/bin/tigcc
ASFLAGS	= -c -Wa,--warn,-I${SYS},--defsym,${CALC}= #-l

LD	= ${TIGCC}/bin/tigcc
LDFLAGS	= --outputbin --flash-os #--optimize-branches --optimize-relocs \
#          --omit-bss-init

# core
OBJ	=  start.o vectors.o main.o entry.o version.o calc.o
# syscalls
OBJ	+= syscall.o sysent.o sys_desc.o sys_proc.o sys_prot.o sys_time.o
OBJ	+= sys_inode.o pipe.o
# devices
OBJ	+= dev.o dev_audio.o dev_flash.o dev_link.o dev_misc.o #dev_vt.o
OBJ	+= dev_tty.o
# helper routines
OBJ	+= drawcell.o smallfont.o kputs.o kputchar.o scrollcell.o
OBJ	+= copy.o fio.o
OBJ	+= setjmp.o m68k.o string.o rdwri.o bio.o trap.o queue.o flash.o
OBJ	+= long.o subr.o tty.o inode.o alloc.o process.o bmap.o
OBJ	+= panic.o abort.o assert.o kprintf.o clock.o lcd.o #globals.o

#OBJ = testvectors.o

all:	punix-9x.tib

punix-9x.tib:	${OBJ}
	${LD} ${LDFLAGS} -o punix $^ #${TIGCC}/lib/flashos.a #${TIGCC}/lib/tigcc.a

obj:	${OBJ}

${OBJ}: Makefile ../sys/globals.h ../sys/punix.h

########################################
# source dependencies
########################################

start.o: start.s
vectors.o: vectors.s
version.o: version.s
entry.o: entry.s ${SYS}/signal.inc
drawcell.o: drawcell.s ${SYS}/lcd.inc
calc.o: calc.s
#switchthread.o:
m68k.o: m68k.s
string.o: string.s
setjmp.o: setjmp.s
#${SYS}/lcd.inc: ${SYS}/lcdglo.inc
#${SYS}/lcd.h: ${SYS}/lcdglo.h
#${SYS}/cell.h: ${SYS}/cellglo.h ${SYS}/lcd.h
#${SYS}/cell.inc: ${SYS}/cellglo.inc ${SYS}/lcd.inc
${SYS}/proc.h: ${SYS}/file.h ${STDINC}/sys/types.h \
	${STDINC}/time.h ${STDINC}/signal.h
${SYS}/process.h: ${SYS}/proc.h

md5.o: md5.c
qsort.o: qsort.c
unpack.o: unpack.c
kputs.o: kputs.c
kputchar.o: kputchar.c ${SYS}/cell.h ${SYS}/scrollcell.h ${SYS}/drawcell.h
syscall.o: syscall.c ${SYS}/sysent.h ${SYS}/process.h
sysent.o: sysent.c ${SYS}/sysent.h
main.o: main.c ${SYS}/proc.h ${STDINC}/assert.h
sys_desc.o: sys_desc.c ${SYS}/proc.h ${SYS}/punix.h ${SYS}/file.h \
		${SYS}/process.h
sys_inode.o: sys_inode.c ${SYS}/inode.h ${SYS}/punix.h ${SYS}/file.h \
		${SYS}/process.h # just guessing the dependencies for now
sys_proc.o: sys_proc.c ${SYS}/proc.h ${SYS}/punix.h ${SYS}/process.h 
sys_prot.o: sys_prot.c
sys_time.o: sys_time.c
panic.o: panic.c ${SYS}/punix.h
process.o: process.c ${SYS}/proc.h ${SYS}/process.h \
		${STDINC}/assert.h ${SYS}/setjmp.h
hanoi.o: hanoi.c
abort.o: abort.c ${SYS}/punix.h
assert.o: assert.c
kprintf.o: kprintf.c
fio.o: fio.c
rdwri.o: rdwri.c
bio.o: bio.c
dev.o: dev.c
bmap.o: bmap.c
trap.o: trap.c
queue.o: queue.c
dev_link.o: dev_link.c
dev_audio.o: dev_audio.c
dev_flash.o: dev_flash.c

#${SYS}/cellglo.h ${SYS}/cellglo.inc ${SYS}/glo.h ${SYS}/glo.inc \
${SYS}/lcdglo.h ${SYS}/lcdglo.inc ${SYS}/procglo.h ${SYS}/procglo.inc \
${SYS}/vectorsglo.h ${SYS}/vectorsglo.inc ${SYS}/heapglo.h ${SYS}/heapglo.inc: \
${SYS}/cellglo.def ${SYS}/glo.def ${SYS}/lcdglo.def \
${SYS}/procglo.def ${SYS}/vectorsglo.def ${SYS}/heapglo.def
#	make -C ${SYS}

########################################
# Data (images, fonts)
########################################

smallfont.o: smallfont.bin
smallfont.bin: fonts/smallfont.pbm
	tail -c 3072 <$< >$@
fonts/smallfont.pbm: fonts/smallfont.ppm
	make -C fonts smallfont.pbm

clean:
	rm -f *.bin *.o punix-9x.tib
	make -C fonts clean
	#make -C ${SYS} clean
