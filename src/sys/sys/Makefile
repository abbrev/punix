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



AWK	= awk

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
SRC	=  start.s vectors.s main.c entry.s version.s calc.s signal.c
# syscalls
SRC	+= syscall.c sysent.c sys_desc.c sys_proc.c sys_prot.c sys_time.c
SRC	+= pipe.c sys_sig.c
# devices
SRC	+= dev.c dev_audio.c dev_flash.c dev_link.c dev_misc.c dev_vt.c
SRC	+= lcd.c dev_tty.c keyscan.c batt.c
# helper routines
SRC	+= kputs.c scroll.s
SRC	+= copy.s fio.c
SRC	+= setjmp.s m68k.s string.s trap.c queue.s flash.s bio.c # rdwri.c
SRC	+= context.s
SRC	+= long.s subr.c tty.c inode.c process.c pfs_alloc.c pfs_bmap.c
SRC	+= panic.c abort.c assert.c kprintf.c printf.c clock.c #globals.c
SRC	+= ffs.c loadav.c drawglyph.s heap.c sched.c bogomips.c
SRC	+= fpuemu.s sysctl.c
SRC	+= userstart.s usermain.c qsort.c mulu64.s divu64.s

#OBJ = testvectors.o

OBJ	:= $(SRC:.c=.o)
OBJ	:= $(OBJ:.s=.o)

TMPUNAME = uname.sysname uname.nodename uname.release uname.version uname.machine stupiddate

.PHONY: all
all:	tib
.PHONY: scratch
scratch: clean all

.PHONY: tib
ifeq ($(CALC), TI92P)
tib: punix-9x.tib
else
tib: punix-89.tib
endif

punix-9x.tib:	${OBJ}
	${LD} ${LDFLAGS} -o punix $^ #${TIGCC}/lib/flashos.a #${TIGCC}/lib/tigcc.a
punix-89.tib:	${OBJ}
	${LD} ${LDFLAGS} -o punix $^ #${TIGCC}/lib/flashos.a #${TIGCC}/lib/tigcc.a

########################################
# generated data
########################################

version.o: ${TMPUNAME}
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
sysent.c: sysent.txt
	${AWK} -f mksysent.awk sysent.txt >sysent.c

glyphsets/%.inc:
	${MAKE} -C glyphsets "`basename $@`"


########################################
# miscellaneous
########################################

.PHONY: clean
clean:
	rm -f *.bin *.o punix-9x.tib punix-89.tib ${TMPUNAME}
	rm -f sysent.c
	${MAKE} -C glyphsets clean
	#${MAKE} -C ${SYSINC} clean

DEPEND = .dep

.PHONY: depend
depend:
	-${TIGCC}/bin/gcc -M -MG ${CFLAGS} ${SRC} >${DEPEND}

${DEPEND}:
	-${TIGCC}/bin/gcc -M -MG ${CFLAGS} ${SRC} >${DEPEND}

-include ${DEPEND}
