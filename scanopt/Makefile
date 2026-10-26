TARGETS=libscanopt.a
INCLUDES=scanopt.h
CFLAGS=-g
STH_DIR=
TEST_CASES=

all: $(TARGETS)

install: all
	mkdir -p ../lib ../include	
	cp $(TARGETS) ../lib/
	cp $(INCLUDES) ../include/

libscanopt.a: scanopt.o
	$(AR) -rcs $@ $<


scanopt.o: scanopt.h scanopt.c

clean:
	$(RM) *.o 
	$(RM) sth/encoded* sth/decoded*
	$(RM) -r a.out* *.dSYM

refresh:
	make clean
	$(RM) $(TARGETS)
