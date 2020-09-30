#! /bin/bash

# This is a small shell script that is part of the prototype implementation the scgi-launch system.
#
# The scgi-launch system is designed to function as an SCGI daemon that, for each request, launches a CGI program.
# By use of the scgi-launch program, you can effectively transform a CGI program into a SCGI server for that program.
#
# In our implementation, we rely on existing programs, stitched together via a bash script, to reduce development time.
#   It is would be straight forward to implement the scgi-launch program in C to improve performance.

# This bash script relies on only three existing programs:
#   - socket: manages the server-side TCP socket, and wires this communication to scgi2env-exec program's stdin/out
#   - scgi2env-exec:  a C program, provided with this project, that performs the following operations:
#       * reads a SCGI header from STDIN
#       * creates an environment structure containing the CGI environment variables
#       * "exec" the user program
#   - "program":   a user supplied program that is passed to this script as it's only arguement.


# The steps associates with the 'scgi-launch' script is as follows:
#    Listens on a socket ${ADDR}:${PORT}, via the 'socket' command
#    Forks the scgi2env-exec program 
#    Loops back to receive an additional network request
#    Reads an SCGI request that has been placed on the wire
#    Creates the CGI environment variables
#    Executes the CGI program

# Usage:  scgi-launch ADDR PORT CGI_PROGRAM

ADDR=$1
PORT=$2 
CGI_PROGRAM=$(readlink -f $3)

SCGI2ENV_EXEC="$(readlink -f $(dirname $0)/scgi2env-exec)"

[ $# == 3 ]             || { echo "Usage: scgi-launch ADDR PORT CGI_PROGRAM" ; exit 1 ; }
[ -f ${SCGI2ENV_EXEC} ] || { echo "Error: scgi2env-exec program not found"   ; exit 1 ; }
[ -x ${CGI_PROGRAM} ]   || { echo "Error: ${CGI_PROGRAM} program is invalid" ; exit 1 ; }




socket -B ${ADDR} -s ${PORT} -b -f -q -l -p "${SCGI2ENV_EXEC} ${CGI_PROGRAM}"
     # Arguments to the socket command:
     #   -B: bind the socket to the ip of ${ADDR}
     #   -s: a server-side socket is created on ${PORT}
     #   -b: background the process as a daemon
     #   -f: fork a child process for each connection
     #   -q: quit: The connection is closed when an end-of-file condition occurs on stdin
     #   -l: loop to receive the next network connection
     #   -p: execute the supporting program: 'scgi2env-exec'





