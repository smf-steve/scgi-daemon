# Project Name: scgi-daemon

## Quick Note: This repository is under current refactoring. 

## Description (upon completion):
This repository contains an implementation of a SCGI server designed to work with a pool of pre-forked child processes.  The purpose of the pre-forked processes is to reduce the (observable) time necessary to to execute a CGI program.  In total, having a dedicated SCGI server allows for the following benefits:

   1. The responsibility of execution CGI script can be offloaded from a web (HTTP) server.
   1. The SCGI server can be located on a backend server, thus reduce load on the front-end web server.
   1. the SCGI server can be located within a docker container, thus providing an additional layer of security
   1. The SCGI has a pool of pre-forked worker, thus reducing the start-time associated with a SCGI server

We also provide performance analysis on the four implementations to gain insight on ways to improve the performance of an SCGI server.  At the start of this project, we surmised that the "fork" operation to create child workers would be the predominate cost in bootstrapping a CGI process.

The four implementation are designed to incrementally improve performance over each previous version by altering on facet of the final implementation.  In total, the four implementations include:

   1. A `command-line` utility implementation
      - To use only command-line tools to build an SCGI server
      - To reduce development time for small project
      - To use the client-server model as opposed to the fork-exec model
        * albeit the 'fork' operation is hidden within the "socket" utility

   1. An implementation using the traditional `accept-fork`-exec model
      - a SCGI request is received (the `accept`)
      - a dedicated child process is then created (the `fork`)
      - the child process then executes the underlying CGI program (the `exec`)

   1. An implementation using the `fork-accept-1`-exec approach
      - a single (`1`) child process is created at server startup time (the `fork`)
      - this child process then accepts the next SCGI request (the `accept`)
      - the child process executes the underlying CGI program (the `exec`)
      - the parent process then created a single child process for the next SCGI request

   1. The final implementation (`fork-accept-pool`) which follows the same model as the `fork-accept-1` model but with a pool of child workers
      - a pool of child processes are created at server startup time
      - each child process, in turn, executes an SCGI request
      - the parent process, upon the completion of child process, replenishes the pool of workers.


## Note:

This project was developed as part of a larger project to study the various ways to generate dynamic content within a web environment. The overall goal of this study is to determine the best way:

  * to provide maximum flexibility to fledgling developers as they develop various web applications
  * to eliminate interdependence between these developers and the administrators of the web environment
  * to increase the level of security by the use of containers.

It was envisioned that each student would develop, maintain, and/or use an independent SCGI server for their work. Each of these students would be assigned a dedicated end-point for their SCGI server; that is to say, they would be assigned a port for the SCGI server. The assigned port would be associated with a docker container that they also maintained.


### Current Activities:

  * [ ] Flesh-out this README.md file on current and future work
  * [ ] Refactor the prototype tool into a pure `command-line` implementation of an SCGI server
    - [ ] moving then netstring decoding code to a separate command-line utility and C library
    - [ ] remove reliance on `sgi2env.c`
    - [ ] perform fd remapping via shell capabilities (as opposed to C's dup2 functions)
  * [ ] Create a command-line tool to encode/decode `netstring`s, based upon a C library
  * [ ] Implement accept-fork model 
  * [ ] Implement fork-accept model
  * [ ] Implement accept-fork-pool model
  * [ ] Create testing procedures using [STH](https://github.com/smf-steve/sth) 
  * [ ] Update the revised repository to work within a docker container  
  * [ ] Create performance timing procedures


### Potential Future Work:

   * Determine the current applicability of the noted larger project. Obtaining the necessary computational resources with the desired/required flexibility for student use might be insurmountable.  At least it was a year after the start of the original prototype work.  This is why the work on this repository was put on hiatus.

   * Rust:
     - to implement the SCGI server in `rust`
     - to perform performance analysis between the `C` and `rust` implementations

   * fcgi-daemon:
     - to ingress the [fcgi-daemon](https://github.com/smf-steve/fcgi-daemon) repository 
     - to clean up said repository
     - to perform performance analysis between fcgi and scgi


## Related Links:
* socket: http://manpages.ubuntu.com/manpages/xenial/en/man1/socket.1.html
* Simple Common Gateway Interface (SCGI):
  * https://en.wikipedia.org/wiki/Simple_Common_Gateway_Interface
  * http://python.ca/scgi/protocol.txt
* Common Gateway Interface (CGI): https://en.wikipedia.org/wiki/Common_Gateway_Interface
* Netstring: https://en.wikipedia.org/wiki/Netstring



# REWORK BELOW:

---

## Installation Methods:
You can install this package either from source or as a docker container.  In both cases, you need to configure the web server to act as a proxy to your SCGI program.  Of course, you can talk directly to your SCGI daemon, but you would need to transmit a valid SCGI request using the wire protocol.

For descriptive purposes, we assume that the web server will proxy the SCGI server at the following URL: `https://hostname.com/${URI_BASE}/${SCGI_NAME}`

During the installation process, you will refer to the following environment variables, which you need to define.
* URI_BASE: the assigned URI path associated with the SCGI daemon.
* SCGI_NAME: the name of the used to identify the SCGI daemon within the URI
* SCGI_TAG: an unique ID for the Docker container. If the SCGI_NAME is globally unique, SCGI_TAG should be set to SCGI_NAME.
* ADDR: the local address associated with the allocated socket, e.g., 'localhost'
* PORT: the assigned port associated with the allocated socket
* CGI_PROGRAM: the user provided CGI program to be executed via an SCGI daemon.


# Apache Server Configuration:
* Enable the proxy module on the Apache server: `sudo a2enmod proxy`
* Enable the proxy_scgi module on the Apache server: `sudo a2enmod proxy_scgi`
* Include a ProxyPass rule for the SCGI daemon.  E.g, `ProxyPass "${URI_BASE}/${SCGI_NAME}" "scgi://${ADDR}:${PORT}/"`
* Restart the Apache server: `sudo service apache2 restart`

## Installation from Source:
```
git clone https://github.com/csuntechlab/scgi-daemon.git
cd scgi-daemon
make  # Compiles the scgi2env-exec.c program, etc.
cd ..
./scgi-daemon/scgi-launch ${ADDR} ${PORT} ${CGI_PROGRAM} 
```
You can access your SCGI daemon directly via the defined ${ADDR} and ${PORT} defined using the socket program. E.g.,

```socket ${ADDR} ${PORT} < ./scgi-daemon/TestingCode/simple.request```

## Docker Installation for ${CGI_PROGRAM}:
```
sudo docker build --tag ${SCGI_TAG} --build-arg PORT=${PORT} https://github.com/csuntechlab/scgi-daemon.git
sudo docker create --name ${SCGI_TAG} --network host --expose ${PORT} -it ${SCGI_TAG} /bin/bash
sudo docker start ${SCGI_TAG}
sudo docker cp ${CGI_PROGRAM} ${SCGI_TAG}:/scgi-daemon/cgi-program
sudo docker exec ${SCGI_TAG} /scgi-daemon/scgi-launch localhost ${PORT} /scgi-daemon/cgi-program
```

### Note:
* In the above installation instructions, we presume we are running a linux host with "host" network.
* With a "bridge" network, you will need to do port mapping:  --host bridge -p {PORT}:{$PORT}
* We need a better way to ingress the CGI_PROGRAM into the Container

# Example
* URL of the Example Program:  https://www.sandbox.csun.edu/~steve/scgi-bin/emit-env
* ProxyPass Directive:  `ProxyPass "/~steve/scgi-bin/" "scgi://localhost:4000/"`
* Define Environment Variables:
```
 URI_BASE=/~steve/scgi-bin/
 SCGI_NAME=emit-env
 SCGI_TAG=emit-env.d
 ADDR=localhost
 PORT=4000
 CGI_PROGRAM=~steve/public_html/cgi-bin/emit-env.cgi 
 ```

   
