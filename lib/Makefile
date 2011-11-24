LIBSRC = ../src/lib

LIBS = libc.a   # add more libs here...

all: $(LIBS)

libc.a: $(LIBSRC)/libc/libc.a

# catch-all rule for libraries
# if the rule has a pre-requisite, copy it to the target
# otherwise call make on the target's directory
%.a:
	if [ -n '$<' ]; then cp $^ . ; \
	else $(MAKE) -C `dirname $@` ; fi

clean:
	rm -f $(LIBS)
	$(MAKE) -C $(LIBSRC) clean

scratch:
	$(MAKE) clean
	$(MAKE) all

.PHONY: all clean scratch
