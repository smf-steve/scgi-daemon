TARGETS=netstring scgi_netstring netstring.a
CFLAGS=-g
STH_DIR=sth
TEST_CASES=cgi_env large one two three four five six seven eight nine

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


netstring: libnetstring.a netstring_utility.o
	$(CC) -o $@ netstring_utility.o -L . -lnetstring

scgi_netstring: libscgi_netstring.a netstring_utility.o
	$(CC) -o $@ -DSCGI_ENCODING netstring_utility.o -L . -lscgi_netstring

netstring_utility.o: netstring_utility.c


libnetstring.a: netstring.o
	$(AR) -rcs $@ $<

libscgi_netstring.a: scgi_netstring.o netstring.o
	$(AR) -rcs $@ $^

netstring.o: netstring.h netstring.c
scgi_netstring.o: scgi_netstring.h scgi_netstring.c

netstring_utility.o: netstring.h netstring_utility.c
scgi_netstring_utility.o: scgi_netstring.h scgi_netstring.c


clean:
	$(RM) *.o 
	$(RM) sth/encoded* sth/decoded*
	$(RM) -r a.out* *.dSYM

refresh:
	make clean
	$(RM) $(TARGETS)
