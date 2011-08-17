LIBS = libc

all: $(LIBS)

$(LIBS):
	$(MAKE) -C $@

clean:
	-for d in $(LIBS); do $(MAKE) -C $$d clean; done

scratch:
	$(MAKE) clean
	$(MAKE) all

.PHONY: all libc clean scratch
