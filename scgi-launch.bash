#! /bin/bash

# This is a small shell script that is part of the prototype implementation of the scgi-launch program.
#
# By use of the scgi-launch program, you can effectively transform a CGI program into a SCGI server for that program.
#
# In our implementation, we rely on exist programs used together in a bash script to reduce development time.
#   It is would be straight forward to implement the scgi-launch program in C to improve performance.

# This base script relies on only three existing programs:
#   - socket: manages the server-side TCP socket, and wires this communication to scgi2env-exec program's stdin/out
#   - scgi2env-exec:  a C program, provided with this project, that performs the following operations:
#       * reads a SCGI header from STDIN
#       * inserts the CGI environment variables into the execution environment
#       * "exec" the user program
#   - "program":   a user supplied program that is passed to this script as it's onl arguement.


# The steps associates with the 'scgi-launch' is as follows:
#    Listens on a socket ${HOSTNAME}:${PORT}, via the 'socket' command
#    Reads an SCGI request
#    Creates the CGI environment variables
#    Executes the CGI program

# Usage:  scgi-launch HOSTNAME PORT PROGRAM

HOSTNAME=$1
PORT=$2 
PROGRAM=$3

[ $# == 3 ] || { echo "Usage:  scgi-launch HOSTNAME PORT PROGRAM" ; exit 1 ; }

socket -B ${HOSTNAME} -s ${PORT}  -l -f -q -p "./scgi2env-exec ${PROGRAM}"
     # Arguments to the socket command:
     #   -B: bind the socket to the local ${HOSTNAME}
     #   -s: a server-side socket is created on ${PORT}, the default is '2020'
     #   -f: fork a child process for each connection
     #   -l: loop to receive additional connections
     #   -q: DO I NEED TO QUIT?
     #   -p: execute the program './scgi2env-exec'
     #       Note that the program cannot be a plain name.




