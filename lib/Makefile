LIBSRC = ../src/lib

all:
	$(MAKE) -C $(LIBSRC)

clean:
	rm -f *.a
	$(MAKE) -C $(LIBSRC) clean

scratch:
	$(MAKE) clean
	$(MAKE) all

.PHONY: all clean scratch
