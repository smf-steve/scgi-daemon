TARGETS=gen-request scgi-launch scgi2env-exec

all: scgi-launch scgi2env-exec


scgi-launch: scgi-launch.bash
	which socket 2>/dev/null || echo "Install socket program"
	cp scgi-launch.bash scgi-launch


scgi2env-exec: scgi2env-exec.c
	$(CC) -o scgi2env-exec scgi2env-exec.c



#socket:
#	apt-get update && apt-get install socket


clean:
	$(RM) $(TARGETS)
	$(RM) *~
	$(RM) \#*




