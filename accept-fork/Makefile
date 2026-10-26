# Makefile for the scgi-daemon using the accept-fork model
TARGETS=scgid
SRCS=scgi_server.c command_line.c
OBJS=scgi_server.o command_line.o
LIBS=scanopt 
CFLAGS=-g -I ../include
STH_DIR=
TEST_CASES=

all: $(TARGETS)


test:	
	@echo TESTING
	@echo ---
	@echo
	@cd sth ; for i in ${TEST_CASES} ; do \
	  echo $$i: ; \
	  echo "----------" ; \
	  ./tester.bash $$i ; \
	  echo ; \
	done


#scgid: $(OBJS)
#	$(CC) -o $@ $(OBJS) 

scgid: $(OBJS)
	$(CC) -o $@ $(OBJS) -I ../include -L ../lib -lscanopt


clean:
	$(RM) *.o 
	$(RM) -r a.out* *.dSYM

refresh:
	make clean
	$(RM) $(TARGETS)
