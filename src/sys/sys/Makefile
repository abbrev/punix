DATE := $(shell date +'%Y-%m-%d %H:%M:%S%z')
REALTIME := $(shell date -d "$(DATE)" +%s)
STUPIDDATE := $(shell date -d "$(DATE)" +%m/%d/%Y)

# configuration
ifeq ($(CALC), )
CALC	= TI92P
endif

UNAME_SYSNAME := Punix
UNAME_NODENAME := timmy
UNAME_RELEASE := 0.08
UNAME_VERSION := $(DATE)
UNAME_MACHINE := m68k

PUNIXBASE	= ../../..

STDINC	= $(PUNIXBASE)/include
SYSINC	= $(PUNIXBASE)/src/sys/h
STDLIB	= $(PUNIXBASE)/lib
LIBC	= $(STDLIB)/libc.a

AWK	= awk

CC	= $(TIGCC)/bin/tigcc
CFLAGS	= -O2 -fomit-frame-pointer -Wall -nostdinc -fno-builtin -mno-bss \
	-I$(SYSINC) -I. -I- -I$(STDINC) -D__KERNEL__ -D$(CALC)= \
	-DSMALLGLYPHS= -DREALTIME=$(REALTIME)UL

AS	= $(TIGCC)/bin/tigcc
ASFLAGS	= -c -Wa,-m68881,--warn,-I$(SYSINC),--defsym,$(CALC)= #-l

LD	= $(TIGCC)/bin/tigcc
LDFLAGS	= -nostdlib -L$(STDLIB) --outputbin --flash-os -lc
#--optimize-branches --optimize-relocs \
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
SRC	+= m68k.s trap.c flash.s bio.c # rdwri.c
SRC	+= context.s
SRC	+= long.s subr.c tty.c inode.c process.c pfs_alloc.c pfs_bmap.c
SRC	+= panic.c abort.c assert.c kprintf.c printf.c clock.c #globals.c
SRC	+= loadav.c drawglyph.s heap.c sched.c bogomips.c
SRC	+= fpuemu.s sysctl.c
SRC	+= userstart.s usermain.c qsort.c mulu64.s divu64.s
#SRC	+= string.s setjmp.s ffs.c

#OBJ = testvectors.o

OBJ	:= $(SRC:.c=.o)
OBJ	:= $(OBJ:.s=.o)
OBJ	+= $(LIBC)

TMPUNAME = uname.sysname uname.nodename uname.release uname.version uname.machine stupiddate

.PHONY: all
all:	tib
.PHONY: scratch
scratch:
	$(MAKE) clean
	$(MAKE) all

%.a:
	$(MAKE) -C `dirname $@`

.PHONY: tib
ifeq ($(CALC), TI92P)
tib: punix-9x.tib
else
tib: punix-89.tib
endif

punix-9x.tib:	$(OBJ)
	$(LD) $(LDFLAGS) -o punix $^
punix-89.tib:	$(OBJ)
	$(LD) $(LDFLAGS) -o punix $^

########################################
# generated data
########################################

version.o: $(TMPUNAME)
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
	$(AWK) -f mksysent.awk sysent.txt >sysent.c || { rm -f sysent.c; exit 1; }

glyphsets/%.inc:
	$(MAKE) -C glyphsets "`basename $@`"


########################################
# miscellaneous
########################################

DEPEND = .dep


.PHONY: depend
depend $(DEPEND):
	@-$(TIGCC)/bin/gcc -M -MG $(CFLAGS) $(SRC) >$(DEPEND) 2>/dev/null

.PHONY: clean
clean:
	rm -f *.bin *.o punix-9x.tib punix-89.tib $(TMPUNAME)
	rm -f sysent.c
	rm -f $(DEPEND)
	$(MAKE) -C glyphsets clean
	#$(MAKE) -C $(SYSINC) clean

include $(DEPEND)

