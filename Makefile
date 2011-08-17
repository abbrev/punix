all: lib kernel filesystem

kernel: punix-9x.tib

filesystem: commands mkpfs

punix-9x.tib: src/sys/sys/punix-9x.tib
	cp $^ .

src/sys/sys/punix-9x.tib: lib
	make -C src/sys/sys depend
	make -C src/sys/sys punix-9x.tib

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
	make -C tools/fs

.PHONY: all lib kernel filesystem clean scratch
