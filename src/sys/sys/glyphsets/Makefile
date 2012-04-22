GLYPHS2C = (echo '{' && tail -c1152 | xxd -i && echo '},')
STATUS2C = (echo '{' && tail -c7 | xxd -i && echo '},')
STATUS2C89 = (echo '{' && tail -c4 | xxd -i && echo '},')

GLYPHS = upper.pbm small-upper.pbm uk.pbm us.pbm small-us.pbm sg.pbm tcs.pbm
STATUS92P = status-none.pbm status-2nd.pbm status-alpha.pbm status-bell.pbm \
            status-capslock.pbm status-compose1.pbm status-compose2.pbm \
            status-diamond.pbm status-hand.pbm status-scrolllock.pbm \
            status-shift.pbm status-batt0.pbm status-batt1.pbm \
            status-batt2.pbm status-batt3.pbm status-batt4.pbm \
            status-busy.pbm status-alphalock.pbm status-handlock.pbm \
	    status-link0.pbm status-link1.pbm status-link2.pbm status-link3.pbm 
STATUS89 = status-none-89.pbm status-2nd-89.pbm status-alpha-89.pbm \
           status-bell-89.pbm status-capslock-89.pbm status-compose1-89.pbm \
           status-compose2-89.pbm status-diamond-89.pbm \
           status-scrolllock-89.pbm status-shift-89.pbm status-batt0-89.pbm \
           status-batt1-89.pbm status-batt2-89.pbm status-batt3-89.pbm \
           status-batt4-89.pbm status-busy-89.pbm status-alphalock-89.pbm \
	   status-link0-89.pbm status-link1-89.pbm status-link2-89.pbm \
	   status-link3-89.pbm 

PBM = $(GLYPHS) $(STATUS92P) $(STATUS89)
INC = $(PBM:.pbm=.inc)

all:
	$(MAKE) $(INC)

clean:
	rm -f *.inc .dep

.dep:
	@-for i in $(GLYPHS); do \
		echo $$i: $$i | sed 's/\.pbm/\.inc/'; \
		echo '	@$$(GLYPHS2C) <$$< >$$@'; \
	done >.dep ; \
	for i in $(STATUS92P); do \
		echo $$i: $$i | sed 's/\.pbm/\.inc/'; \
		echo '	@$$(STATUS2C) <$$< >$$@'; \
	done >>.dep ; \
	for i in $(STATUS89); do \
		echo $$i: $$i | sed 's/\.pbm/\.inc/'; \
		echo '	@$$(STATUS2C89) <$$< >$$@'; \
	done >>.dep ; \

%.inc: %.pbm

include .dep

.PHONY: all clean
