PBM2C = (tail -c 2304 | ./bin2c)

all: bin2c uk.inc us.inc sg.inc

bin2c: bin2c.c

uk.inc: uk.pbm bin2c
	${PBM2C} <$< >$@
us.inc: us.pbm bin2c
	${PBM2C} <$< >$@
sg.inc: sg.pbm bin2c
	${PBM2C} <$< >$@
