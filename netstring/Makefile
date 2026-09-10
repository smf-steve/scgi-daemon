TARGETS=netstring scgi_netstring netstring.a
CFLAGS=-g

all: 
	echo $(TARGETS)

# unable to link via the command: cc -o netstring netstring_utility.o -L. -lnetstring
netstring: netstring.a netstring_utility.o
	$(CC) -o $@ netstring_utility.o netstring.a

scgi_netstring: netstring.a scgi_netstring.a netstring_utility.c
	$(CC) -o $@ -DSCGI_ENCODING netstring_utility.c netstring.a scgi_netstring.a


netstring.a: netstring.o
	$(AR) -rcs $@ $<

netstring.c: netstring.h
scgi_nestring.c: netstring.h scgi_netstring.h

netstring_utility.c: netstring.h
scgi_netstring_utility.c: netstring.h scgi_netstring.h	


clean:
	$(RM) *.o

refresh:
	make clean
	$(RM) $(TARGETS)
