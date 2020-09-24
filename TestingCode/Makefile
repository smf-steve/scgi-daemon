all: gen-request


emit-scgi-env.cgi:
	# returns only the SCGI variables passed along to the target program

emit-env.cgi:
	# returns all the HTTP, SCGI, and CGI variables passed along to the target program


test_stdin: gen-request 
	./gen-request | ../scgi2env-exec ./emit-scgi-env.cgi


test_socket: gen-request 
	../scgi-launch $(shell hostname) 2020 ./emit-env.cgi &
	gen-request | socket $(shell hostname) 2020


gen-request: gen-request.c
	# A program to generate the binary format of a SCGI request
	$(CC) -o gen-request gen-request.c


simple.request:
	# A simple SCGI request generated from gen-request.c
	gen-request > simple.request


www.sandbox.csun.edu.request:
	# A captured SCGI request from from the URL: https://www.sandbox.csun.edu/~steve/scgi-bin
	# Apache config on 'ssh.sandbox.csun.edu': ProxyPass "/~steve/scgi-bin/" "scgi://localhost:4000/"
	# Server side (ssh.sandbox.csun.edu)       socket -B localhost -s 4000 > www.sandbox.csun.edu.request
	# client:                                  curl https://www.sandbox.csun.edu/~steve/scgi-bin
