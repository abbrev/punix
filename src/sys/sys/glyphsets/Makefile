PBM2C = (tail -c 1152 | ./bin2c)

all: bin2c upper.inc uk.inc us.inc small-us.inc sg.inc tcs.inc
	touch all
clean:
	rm -f *.inc all bin2c

bin2c: bin2c.c

upper.inc: upper.pbm bin2c
	${PBM2C} <$< >$@
uk.inc: uk.pbm bin2c
	${PBM2C} <$< >$@
us.inc: us.pbm bin2c
	${PBM2C} <$< >$@
small-us.inc: small-us.pbm bin2c
	${PBM2C} <$< >$@
sg.inc: sg.pbm bin2c
	${PBM2C} <$< >$@
tcs.inc: tcs.pbm bin2c
	${PBM2C} <$< >$@
