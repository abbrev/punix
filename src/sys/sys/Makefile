DATE := $(shell date +'%Y-%m-%d %H:%M:%S%z')
REALTIME := $(shell date -d "$(DATE)" +%s)
STUPIDDATE := $(shell date -d "$(DATE)" +%m/%d/%Y)

# configuration
CALC = TI92P
HOSTNAME := timmy

UNAME_SYSNAME := Punix
UNAME_NODENAME := $(HOSTNAME)
UNAME_RELEASE := 0.08
UNAME_VERSION := $(DATE)
UNAME_MACHINE := m68k

PATH := $(TIGCC):$(TIGCC)/bin:$(PATH)
PUNIXBASE = ../../..
TARGET = $(PUNIXBASE)/target

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
	-DSMALLGLYPHS= # -DREALTIME=$(REALTIME)UL

AS	= "$(TIGCC)/bin/tigcc"
ASFLAGS	= -c -Wa,-m68881,--warn,-I$(SYSINC),--defsym,$(CALC)=,--defsym,REALTIME=$(REALTIME) #-l

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
CSRC	+= sysctl.c usermain.c qsort.c gray.c
SSRC	+= userstart.s mulu64.s divu64.s drawglyph.s

#OBJ = testvectors.o

OBJ	+= $(SSRC:.s=.o)
OBJ	+= $(CSRC:.c=.o)
OBJ	+= $(LIBC)

TMPUNAME = uname.sysname uname.nodename uname.release uname.version uname.machine stupiddate

ifeq ($(CALC), TI92P)
TIB = punix-9x.tib
XXU = punix-9x.9xu
KEY = 01
else ifeq ($(CALC), TI89)
TIB = punix-89.tib
XXU = punix-89.89u
KEY = 03
else
$(error Unsupported CALC: $(CALC))
endif


.PHONY: all
all:	xxu
.PHONY: scratch
scratch:
	$(MAKE) clean
	$(MAKE) depend
	$(MAKE) all

%.a:
	$(MAKE) -C `dirname $@`

# preprocessing only
%.i: %.c
	@echo "  CC $@"
	@$(CC) -E $(CFLAGS) -o $@ $<

%.o: %.c
	@echo "  CC $@"
	@$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.s
	@echo "  AS $@"
	@$(AS) -c $(ASFLAGS) -o $@ $<

.PHONY: tib
tib:
	@echo "  LD $@"
	@$(LD) $(LDFLAGS) -o punix $^

$(TIB): $(OBJ)
	@echo "  LD $@"
	@$(LD) $(LDFLAGS) -o punix $^
	#cp $@ $(PUNIXBASE)

xxu: $(XXU)
%.9xu %.89u: %.tib
	tib2xxu $< $@
	rabbitsign -r -K $(KEY) $@ -o $@
	cp $@ $(PUNIXBASE)

########################################
# generated data
########################################

# build uname every time
.PHONY: $(TMPUNAME) 
version.o: $(TMPUNAME)
uname.sysname:
	@echo -n "$(UNAME_SYSNAME)" >$@
uname.nodename:
	@echo -n "$(UNAME_NODENAME)" >$@
uname.release:
	@echo -n "$(UNAME_RELEASE)" >$@
uname.version:
	@echo -n "$(UNAME_VERSION)" >$@
uname.machine:
	@echo -n "$(UNAME_MACHINE)" >$@
stupiddate:
	@echo -n "$(STUPIDDATE)" >$@
sysent.c: sysent.txt
	$(AWK) -f mksysent.awk sysent.txt >sysent.c || { rm -f sysent.c; exit 1; }

glyphsets/%.inc:
	$(MAKE) -C glyphsets "`basename $@`" >/dev/null


########################################
# miscellaneous
########################################

DEPEND = .dep


.PHONY: depend
depend $(DEPEND):
	@-$(CC) -M -MG $(CFLAGS) $(CSRC) >$(DEPEND) #2>/dev/null

.PHONY: clean
clean:
	rm -f *.bin *.o *.tib *.[89]?u $(TMPUNAME)
	rm -f sysent.c
	rm -f $(DEPEND)
	$(MAKE) -C glyphsets clean
	#$(MAKE) -C $(SYSINC) clean

include $(DEPEND)

