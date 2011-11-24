all: lib kernel filesystem

PLATFORMS = ti89 ti92p
# build kernels for all platforms
kernel:
	for p in $(PLATFORMS); do $(MAKE) $$p; done

ti89: punix-89.tib
ti92p: punix-9x.tib

filesystem: commands mkpfs

punix-89.tib: src/sys/sys/punix-89.tib
	cp $^ .
punix-9x.tib: src/sys/sys/punix-9x.tib
	cp $^ .

src/sys/sys/punix-89.tib: lib
	$(MAKE) -C src/sys/sys CALC=TI89 scratch

src/sys/sys/punix-9x.tib: lib
	$(MAKE) -C src/sys/sys CALC=TI92P scratch

lib:
	$(MAKE) -C lib

clean:
	$(MAKE) -C lib clean
	$(MAKE) -C src/sys/sys clean

scratch:
	$(MAKE) clean
	$(MAKE) all

# TODO
commands:

mkpfs: tools/fs/mkpfs

tools/fs/mkpfs:
	$(MAKE) -C tools/fs

.PHONY: all lib kernel filesystem clean scratch
