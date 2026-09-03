# Although this file is structured as a Makefile and can be used as such,
# the file is used to help document some of testing steps.

all: gen-request

gen-request: gen-request.c
	# A program to generate the binary format of an example SCGI request
	$(CC) -o gen-request gen-request.c

simple.request:
	# A simple SCGI request generated from gen-request.c
	gen-request > simple.request


www.sandbox.csun.edu.request:
	# A captured SCGI request from the URL: https://www.sandbox.csun.edu/~steve/scgi-bin
	# Apache config on 'ssh.sandbox.csun.edu': ProxyPass "/~steve/scgi-bin/" "scgi://localhost:4000/"
	# Server side (ssh.sandbox.csun.edu)       socket -B localhost -s 4000 > www.sandbox.csun.edu.request
	# client:                                  curl https://www.sandbox.csun.edu/~steve/scgi-bin


emit-scgi-env.cgi:
	# A test program that can be provided to ../scgi-launch
	# returns only the SCGI variables passed along to the target program

emit-env.cgi:
	# A test program that can be provided to ../scgi-launch
	# returns all the HTTP, SCGI, and CGI variables passed along to the target program


test_stdin: gen-request
	# The command used to test scgi2env from the CLI using stdin
	./gen-request | ../scgi2env-exec ./emit-scgi-env.cgi


test_socket: gen-request
	# The command used to test scgi2env as it is being called via ../scgi-launch locally
	../scgi-launch $(shell hostname) 2020 ./emit-env.cgi &
	gen-request | socket $(shell hostname) 2020

test_deployed:
	# Ensure a ProxyPass has been setup, e.g. ProxyPass $URI scgi://$SCGIHOST:$PORT/"
	# On server $WEBHOST: ../scgi-launch $SCGIHOST $PORT ./emit-env.cgi
	# On client: curl https://$WEBHOST/$URI





clean:
	$(RM) $(TARGETS)
	$(RM) *~
	$(RM) \#*

