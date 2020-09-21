TARGETS=gen-request scgi-launch scgi2env-exec

all: scgi-launch scgi2env-exec


scgi-launch: scgi-launch.bash
	which socket   # validate that socket has been installed
	cp scgi-launch.bash scgi-launch


scgi2env-exec: scgi2env-exec.c
	$(CC) -o scgi2env-exec scgi2env-exec.c


# Targets to make a simple test

test_std: gen-request scgi2env-exec
	./gen-request | ./scgi2env-exec set

test_socket: gen-request scgi-launch
	scgi-launch $(shell hostname) 2020 set &
	gen-request | socket $(shell hostname) 2020

gen-request: gen-request.c
	$(CC) -o gen-request gen-request.c



#socket:
#	apt-get update && apt-get install socket


clean:
	$(RM) $(TARGETS)
	$(RM) *~
	$(RM) \#*




