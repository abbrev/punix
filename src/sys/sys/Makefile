# base address of RAM
RAMBASE	= 0x4C00

ALL	= cellglo.h cellglo.inc glo.h glo.inc lcdglo.h lcdglo.inc procglo.h \
	procglo.inc vectorsglo.h vectorsglo.inc heapglo.h heapglo.inc
all: ${ALL}

${ALL}: cellglo.def glo.def lcdglo.def procglo.def vectorsglo.def heapglo.def
	./mksymdefs.sh ${RAMBASE} vectorsglo lcdglo glo cellglo procglo heapglo

#proc.inc: proc.h
#	@echo '*** you might need to update proc.inc! ***'
#	exit 1

clean:
	rm -f *glo.{h,inc}
