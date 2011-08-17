PUNIXBASE	= ../../..

STDINC	= ${PUNIXBASE}/include

CC	= ${TIGCC}/bin/tigcc
CFLAGS	= -O2 -fomit-frame-pointer -Wall -nostdinc -fno-builtin -mno-bss \
	-I. -I- -I${STDINC}

AS	= ${TIGCC}/bin/tigcc
ASFLAGS	= -c -Wa,--warn #-l

LD	= ${TIGCC}/bin/tigcc
LDFLAGS	=

AR	= $(TIGCC)/bin/ar-tigcc

all:	libc.a
scratch:
	$(MAKE) clean
	$(MAKE) all

SRC = $(wildcard *.s *.c)
OBJ := $(SRC:.c=.o)
OBJ := $(OBJ:.s=.o)

libc.a: $(OBJ)
	$(AR) -rc libc.a $^

########################################
# miscellaneous
########################################

DEPEND = .dep


depend $(DEPEND):
	@-${TIGCC}/bin/gcc -M -MG ${CFLAGS} ${SRC} >${DEPEND} 2>/dev/null

clean:
	rm -f *.bin *.o *.a
	rm -f $(DEPEND)

-include ${DEPEND}

.PHONY: all depend clean scratch
