all: lib kernel filesystem

PLATFORMS = ti89 ti92p
# build kernels for all platforms
kernel:
	for p in $(PLATFORMS); do $(MAKE) $$p; done

ti89:
	$(MAKE) -C src/sys/sys CALC=TI89 scratch
ti92p:
	$(MAKE) -C src/sys/sys CALC=TI92P scratch

lib:
	$(MAKE) -C lib

clean:
	$(MAKE) -C lib clean
	$(MAKE) -C src/sys/sys clean

scratch:
	$(MAKE) clean
	$(MAKE) all

# make a release!
# this should be made against a clean copy of the source so it doesn't
# include anything embarrassing!
.PHONY: release
release:
	$(MAKE) tgz
	$(MAKE) kernel
	$(MAKE) filesystem

TGZFILE = punix

# tar+gzip all files into punix.tgz, excluding punix.tgz itself
# and VCS files, and prefix filenames with punix/
.PHONY: tgz
tgz:
	tar --exclude-vcs --exclude=*.tgz --xform='s,^,$(TGZFILE)/,' -czf $(TGZFILE).tgz *


# TODO
filesystem: commands mkpfs

commands:

mkpfs: tools/fs/mkpfs

tools/fs/mkpfs:
	$(MAKE) -C tools/fs

.PHONY: all lib kernel filesystem clean scratch
