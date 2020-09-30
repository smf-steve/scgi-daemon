# Project Name: scgi-daemon

# Quick Description:
A prototype implementation of a SCGI server.  This project utilizes the 'socket' command to manage the server components, and a C program (scgi2env-exec) to read the SCGI wire protocol, to create a CGI environment, and to exec the desired CGI program.

## Related Links:
* socket: http://manpages.ubuntu.com/manpages/xenial/en/man1/socket.1.html
* Simple Common Gateway Interface (SCGI):
  * https://en.wikipedia.org/wiki/Simple_Common_Gateway_Interface
  * http://python.ca/scgi/protocol.txt
* Common Gateway Interface (CGI): https://en.wikipedia.org/wiki/Common_Gateway_Interface

# PURPOSE:
*	To allow any CGI program to be invoked under the client-server model as opposed to the fork-exec model
* To containize the CGI program along with the scgi-daemon, to provide a layer of security
* To execute the container either directly on a/the web-server or on a backend server

## Note:
This project was developed as part of a larger project to study the various ways to generate dynamic content within a web environment. The overall goal of this study is to determine the best way:
* to provide maximum flexiblity to fledgling developers as they develop various web applications
* to eliminate interdependence between these developers and the adminstrators of the web environment
* to increase the level of security by the use of containers.

# Architecture
![Architectural Diagram of the SGI Daemon](/images/architecture.png)

# Installation Methods:
You can install this package either from source or as a docker container.  In both cases, you need to configure the web server to act as a proxy to your SCGI program.  Of course, you can talk directly to your SCGI daemon, but you would need to transmit a valid SCGI request using the wireprotocol.

For descriptive purposes, we assume that the webserver will proxy the SCGI server at the following URL: `https://hostname.com/${URI_BASE}/${SCGI_NAME}`

During the installation process, you will refer to the following environment variables, which you need to define.
* URI_BASE: the assigned URI path associated with the SCGI deamon.
* SCGI_NAME: the name of the used to identify the SCGI daemon within the URI
* SCGI_TAG: an unique ID for the Docker container. If the SCGI_NAME is globally unique, SCGI_TAG should be set to SCGI_NAME.
* ADDR: the local address associated with the allocated socket, e.g., 'localhost'
* PORT: the assigned port associated with the allocated socket
* CGI_PROGRAM: the user provided CGI program to be executed via an SCGI daemon.


# Apache Server Configuration:
* Enable the proxy module on the Apache server: `sudo a2enmod proxy`
* Enable the proxy_scgi module on the Apache server: `sudo a2enmod proxy_scgi`
* Include a ProxyPass rule for the SCGI deamon.  E.g, `ProxyPass "${URI_BASE}/${SCGI_NAME}" "scgi://${ADDR}:${PORT}/"`
* Restart the Apache server: `sudo service apache2 restart`

## Installation from Source:
```
git clone https://github.com/csuntechlab/scgi-daemon.git
cd scgi-daemon
make  # Compiles the scgi2env-exec.c program, etc.
cd ..
./scgi-daemon/scgi-launch ${ADDR} ${PORT} ${CGI_PROGRAM} 
```
You don't access your SCGI daemon directly via the ${ADDR} and ${PORT} defined, using the socket program. E.g.,

```socket ${ADDR} ${PORT} << SCGI_request.binary```

## Docker Installation for $CGI_PROGRAM:
```
docker build -t ${SCGI_ID} https://github.com/csuntechlab/scgi-daemon.git
docker create --name ${SCGI_ID} -p ${PORT}:8080 ${SCGI_ID}
docker copy ${CGI_PROGRAM} ${SCGI_ID}:/scgi-daemon/cgi-program
docker start ${SCGI_ID}
```


# Example
* URL of the Example Program:  https://www.sandbox.csun.edu/~steve/scgi-bin/emit-env
* ProxyPass Directive:  `ProxyPass "/~steve/scgi-bin/" "scgi://localhost:4000/"`
* Define Environment Variables:
  * URI_BASE=/~steve/scgi-bin/
  * SCGI_NAME=emit-env
  * SCGI_TAG=emit-env.d
  * ADDR=localhost
  * POR=4000
  * CGI_PROGRAM=emit-env.cgi # (source https://www.sandbox.csun.edu/~steve/cgi-bin/cat.cgi?emit-env.cgi)
 Note that the host server for this example is ssh.sandbox.csun.edu.  www.sandbox.csun.edu function as a reverse proxy.


# Enhancements:
* Merge the socket and scgi2env-exec into a single executable
* Enhance the scgi-daemon to have a number worker threads/process to increase performance
* Modify the process to all the first path component in the URI to identify the name of the CGI program
* Retool the project to use [podman](http://docs.podman.io/en/latest/)
* Update the Dockerfile to build the scgi2env-exec binary under a multistage approach.

# Performance Numbers:
* Generate performance numbers comparing SCGI-daemon with docker.cgi


   
