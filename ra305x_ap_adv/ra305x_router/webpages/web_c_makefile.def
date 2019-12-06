# DO NOT EDIT 

include .objfiles
#ALL_OBJS := WEB_OBJ

all: web_c.o

include $(TOPDIR)/rules.mak

web_c.o: $(WEB_OBJ:.o=.d)
	$(XLD) -r $(ENDIAN) -o web_c.o $(WEB_OBJ)
	@touch web_c.d


