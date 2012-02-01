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
PATH := $(TIGCC):$(TIGCC)/bin:$(PATH)

STDINC	= $(PUNIXBASE)/include
SYSINC	= $(PUNIXBASE)/src/sys/h
STDLIB	= $(PUNIXBASE)/lib
LIBC	= $(STDLIB)/libc.a

AWK	= awk

# "$(TIGCC)/bin/tigcc" doesn't work in Windows because it apparently doesn't
# understand the -nostdinc option (among other options)
#CC	= "$(TIGCC)/bin/tigcc"
CC	= "$(TIGCC)/bin/gcc"
CFLAGS	= -O2 -fomit-frame-pointer -Wall -nostdinc -fno-builtin -mno-bss \
	-I$(SYSINC) -I. -I- -I$(STDINC) -D__KERNEL__ -D$(CALC)= \
	-DSMALLGLYPHS= -DREALTIME=$(REALTIME)UL

AS	= "$(TIGCC)/bin/tigcc"
ASFLAGS	= -c -Wa,-m68881,--warn,-I$(SYSINC),--defsym,$(CALC)= #-l

LD	= "$(TIGCC)/bin/tigcc"
LDFLAGS	= -nostdlib -L$(STDLIB) --outputbin --flash-os
#--optimize-branches --optimize-relocs \
#          --omit-bss-init

# core
SSRC	+= start.s vectors.s entry.s version.s calc.s
CSRC	+= main.c signal.c
# syscalls
CSRC	+= syscall.c sysent.c sys_desc.c sys_proc.c sys_prot.c sys_time.c
CSRC	+= pipe.c sys_sig.c
# devices
CSRC	+= dev.c dev_audio.c dev_flash.c dev_link.c dev_misc.c dev_vt.c
CSRC	+= lcd.c dev_tty.c keyscan.c batt.c
# helper routines
CSRC	+= kputs.c fio.c
CSRC	+= trap.c bio.c # rdwri.c
SSRC	+= flash.s scroll.s copy.s m68k.s
SSRC	+= context.s long.s fpuemu.s
CSRC	+= subr.c tty.c inode.c process.c pfs_alloc.c pfs_bmap.c
CSRC	+= panic.c abort.c assert.c kprintf.c printf.c clock.c #globals.c
CSRC	+= loadav.c heap.c sched.c bogomips.c
CSRC	+= sysctl.c usermain.c qsort.c
SSRC	+= userstart.s mulu64.s divu64.s drawglyph.s

#OBJ = testvectors.o

OBJ	+= $(CSRC:.c=.o)
OBJ	+= $(SSRC:.s=.o)
OBJ	+= $(LIBC)

TMPUNAME = uname.sysname uname.nodename uname.release uname.version uname.machine stupiddate

.PHONY: all
all:	tib
.PHONY: scratch
scratch:
	$(MAKE) clean
	$(MAKE) depend
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

#keyscan.o: glyphsets
#dev_vt.o: glyphsets

.PHONY: glyphsets
glyphsets:
	$(MAKE) -C glyphsets
glyphsets/%.inc:
	$(MAKE) -C glyphsets "`basename $@`"


########################################
# miscellaneous
########################################

DEPEND = .dep


.PHONY: depend
depend $(DEPEND):
	-$(CC) -M -MG $(CFLAGS) $(CSRC) >$(DEPEND) #2>/dev/null

.PHONY: clean
clean:
	rm -f *.bin *.o punix-9x.tib punix-89.tib $(TMPUNAME)
	rm -f sysent.c
	rm -f $(DEPEND)
	$(MAKE) -C glyphsets clean
	#$(MAKE) -C $(SYSINC) clean

include $(DEPEND)

