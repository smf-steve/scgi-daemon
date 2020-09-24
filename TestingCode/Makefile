all: gen-request


test_stdin: gen-request scgi2env-exec
	./gen-request | ./scgi2env-exec ./emit-scgi-env.cgi


test_socket: gen-request scgi-launch
	scgi-launch $(shell hostname) 2020 ./emit-env.cgi &
	gen-request | socket $(shell hostname) 2020


gen-request: gen-request.c
	$(CC) -o gen-request gen-request.c


