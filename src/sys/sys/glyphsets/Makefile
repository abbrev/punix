GLYPHS2C = (tail -c 1152 | ./bin2c)
STATUS2C = (tail -c 8 | ./bin2c)

all: bin2c upper.inc small-upper.inc uk.inc us.inc small-us.inc sg.inc tcs.inc \
     status-none.inc status-2nd.inc status-alpha.inc status-bell.inc \
     status-capslock.inc status-compose1.inc status-compose2.inc \
     status-diamond.inc status-hand.inc status-shift.inc status-scrolllock.inc \
     status-batt0.inc status-batt1.inc status-batt2.inc status-batt3.inc status-batt4.inc 
	touch all
clean:
	rm -f *.inc all bin2c

bin2c: bin2c.c

upper.inc: upper.pbm bin2c
	${GLYPHS2C} <$< >$@
small-upper.inc: small-upper.pbm bin2c
	${GLYPHS2C} <$< >$@
uk.inc: uk.pbm bin2c
	${GLYPHS2C} <$< >$@
us.inc: us.pbm bin2c
	${GLYPHS2C} <$< >$@
small-us.inc: small-us.pbm bin2c
	${GLYPHS2C} <$< >$@
sg.inc: sg.pbm bin2c
	${GLYPHS2C} <$< >$@
tcs.inc: tcs.pbm bin2c
	${GLYPHS2C} <$< >$@

status-none.inc: status-none.pbm bin2c
	${STATUS2C} <$< >$@
status-2nd.inc: status-2nd.pbm bin2c
	${STATUS2C} <$< >$@
status-alpha.inc: status-alpha.pbm bin2c
	${STATUS2C} <$< >$@
status-bell.inc: status-bell.pbm bin2c
	${STATUS2C} <$< >$@
status-capslock.inc: status-capslock.pbm bin2c
	${STATUS2C} <$< >$@
status-compose1.inc: status-compose1.pbm bin2c
	${STATUS2C} <$< >$@
status-compose2.inc: status-compose2.pbm bin2c
	${STATUS2C} <$< >$@
status-diamond.inc: status-diamond.pbm bin2c
	${STATUS2C} <$< >$@
status-hand.inc: status-hand.pbm bin2c
	${STATUS2C} <$< >$@
status-scrolllock.inc: status-scrolllock.pbm bin2c
	${STATUS2C} <$< >$@
status-shift.inc: status-shift.pbm bin2c
	${STATUS2C} <$< >$@
status-batt0.inc: status-batt0.pbm bin2c
	${STATUS2C} <$< >$@
status-batt1.inc: status-batt1.pbm bin2c
	${STATUS2C} <$< >$@
status-batt2.inc: status-batt2.pbm bin2c
	${STATUS2C} <$< >$@
status-batt3.inc: status-batt3.pbm bin2c
	${STATUS2C} <$< >$@
status-batt4.inc: status-batt4.pbm bin2c
	${STATUS2C} <$< >$@
