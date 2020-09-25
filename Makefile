TARGETS=scgi-launch scgi2env-exec

all: scgi-launch scgi2env-exec


scgi-launch: scgi-launch.bash
	which socket 2>/dev/null || { echo "Install socket program" ; false ; }
	cp scgi-launch.bash scgi-launch


scgi2env-exec: scgi2env-exec.c
	$(CC) -o scgi2env-exec scgi2env-exec.c


clean:
	$(RM) $(TARGETS)
	$(RM) *~
	$(RM) \#*




