# This Makefile does not list all the dependencies of each object file, so
# usually it is best to 'make clean' before making the TIB file.

CALC	= TI92P

PUNIXBASE	= ../../..

STDINC	= ${PUNIXBASE}/include
SYSINC	= ${PUNIXBASE}/src/sys/h

CC	= ${TIGCC}/bin/tigcc
CFLAGS	= -O2 -fomit-frame-pointer -Wall -nostdinc -fno-builtin -mno-bss \
	-I${SYSINC} -I. -I- -I${STDINC} -D__KERNEL__ -DBUILD=\"`date +%FT%T`\"

AS	= ${TIGCC}/bin/tigcc
ASFLAGS	= -c -Wa,--warn,-I${SYSINC},--defsym,${CALC}= #-l

LD	= ${TIGCC}/bin/tigcc
LDFLAGS	= --outputbin --flash-os #--optimize-branches --optimize-relocs \
#          --omit-bss-init

# core
OBJ	=  start.o vectors.o main.o entry.o version.o calc.o signal.o
# syscalls
OBJ	+= syscall.o sysent.o sys_desc.o sys_proc.o sys_prot.o sys_time.o
OBJ	+= pipe.o sys_sig.o
# devices
OBJ	+= dev.o dev_audio.o dev_flash.o dev_link.o dev_misc.o #dev_vt.o
OBJ	+= lcd.o dev_tty.o test1.o
# helper routines
OBJ	+= drawcell.o smallfont.o kputs.o kputchar.o scrollcell.o
OBJ	+= copy.o fio.o
OBJ	+= setjmp.o m68k.o string.o rdwri.o bio.o trap.o queue.o flash.o
OBJ	+= long.o subr.o tty.o inode.o alloc.o process.o bmap.o
OBJ	+= panic.o abort.o assert.o kprintf.o clock.o #globals.o
#OBJ	+= ffs.o loadav.o

#OBJ = testvectors.o

scratch: clean all
all:	punix-9x.tib

punix-9x.tib:	${OBJ}
	${LD} ${LDFLAGS} -o punix $^ #${TIGCC}/lib/flashos.a #${TIGCC}/lib/tigcc.a

obj:	${OBJ}

${OBJ}: Makefile ${SYSINC}/globals.h ${SYSINC}/punix.h

########################################
# source dependencies
########################################

start.o: start.s
vectors.o: vectors.s
version.o: version.s
entry.o: entry.s ${SYSINC}/signal.inc
drawcell.o: drawcell.s ${SYSINC}/lcd.inc
calc.o: calc.s
#switchthread.o:
m68k.o: m68k.s
string.o: string.s
setjmp.o: setjmp.s
#${SYSINC}/lcd.inc: ${SYSINC}/lcdglo.inc
#${SYSINC}/lcd.h: ${SYSINC}/lcdglo.h
#${SYSINC}/cell.h: ${SYSINC}/cellglo.h ${SYSINC}/lcd.h
#${SYSINC}/cell.inc: ${SYSINC}/cellglo.inc ${SYSINC}/lcd.inc
${SYSINC}/proc.h: ${SYSINC}/file.h ${STDINC}/sys/types.h \
	${STDINC}/time.h ${STDINC}/signal.h
${SYSINC}/process.h: ${SYSINC}/proc.h

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
		${STDINC}/assert.h ${SYSINC}/setjmp.h
hanoi.o: hanoi.c
abort.o: abort.c ${SYSINC}/punix.h
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

#${SYSINC}/cellglo.h ${SYSINC}/cellglo.inc ${SYSINC}/glo.h ${SYSINC}/glo.inc \
${SYSINC}/lcdglo.h ${SYSINC}/lcdglo.inc ${SYSINC}/procglo.h ${SYSINC}/procglo.inc \
${SYSINC}/vectorsglo.h ${SYSINC}/vectorsglo.inc ${SYSINC}/heapglo.h ${SYSINC}/heapglo.inc: \
${SYSINC}/cellglo.def ${SYSINC}/glo.def ${SYSINC}/lcdglo.def \
${SYSINC}/procglo.def ${SYSINC}/vectorsglo.def ${SYSINC}/heapglo.def
#	make -C ${SYSINC}

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
	#make -C ${SYSINC} clean
