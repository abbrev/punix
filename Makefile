.PHONY: all lib kernel filesystem clean scratch

all: lib kernel filesystem

kernel: punix-9x.tib

filesystem: commands mkpfs

punix-9x.tib: src/sys/sys/punix-9x.tib
	cp $^ .

src/sys/sys/punix-9x.tib:
	make -C src/sys/sys depend
	make -C src/sys/sys punix-9x.tib

lib:
	[ -d lib ] || mkdir lib
	$(MAKE) -C src/lib
	cp src/lib/*/*.a lib/

clean:
	$(MAKE) -C src/lib clean
	$(MAKE) -C src/sys/sys clean

scratch: clean all

# TODO
commands:

mkpfs: tools/fs/mkpfs

tools/fs/mkpfs:
	make -C tools/fs
