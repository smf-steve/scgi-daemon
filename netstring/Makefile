TARGETS=netstring scgi_netstring netstring.a
CFLAGS=-g
STH_DIR=sth
TEST_CASES=cgi_env large one two three four five six seven eight nine

all: $(TARGETS)
	@echo TESTING
	@echo ---
	@echo
	@cd sth ; for i in ${TEST_CASES} ; do \
	  echo $$i: ; \
	  echo "----------" ; \
	  ./tester.bash $$i ; \
	  echo ; \
	done


# unable to link via the command: cc -o netstring netstring_utility.o -L. -lnetstring
netstring: netstring.a netstring_utility.o
	$(CC) -o $@ netstring_utility.o netstring.a

scgi_netstring: netstring.a scgi_netstring.a netstring_utility.o
	$(CC) -o $@ -DSCGI_ENCODING netstring_utility.o netstring.a scgi_netstring.a

netstring_utility.o: netstring_utility.c


netstring.a: netstring.o
	$(AR) -rcs $@ $<

scgi_netstring.a: scgi_netstring.o netstring.o
	$(AR) -rcs $@ $<

netstring.c: netstring.h
scgi_nestring.c: netstring.h scgi_netstring.h

netstring_utility.c: netstring.h
scgi_netstring_utility.c: netstring.h scgi_netstring.h	


clean:
	$(RM) *.o 
	$(RM) sth/encoded* sth/decoded*
	$(RM) -r a.out* *.dSYM

refresh:
	make clean
	$(RM) $(TARGETS)
