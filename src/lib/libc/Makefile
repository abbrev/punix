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

.PHONY: all
all:	libc.a
.PHONY: scratch
scratch: clean all

SRC = $(wildcard *.s *.c)
OBJ := $(SRC:.c=.o)
OBJ := $(OBJ:.s=.o)

libc.a: $(OBJ)
	$(AR) -rc libc.a $^

########################################
# miscellaneous
########################################

DEPEND = .dep


.PHONY: depend
depend $(DEPEND):
	@-${TIGCC}/bin/gcc -M -MG ${CFLAGS} ${SRC} >${DEPEND} 2>/dev/null

.PHONY: clean
clean:
	rm -f *.bin *.o *.a
	rm -f $(DEPEND)

-include ${DEPEND}

