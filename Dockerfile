FROM ubuntu

RUN apt-get clean && apt-get autoremove
RUN apt-get update && apt-get install socket

LABEL project="SCGI Daemon"launch-scgi
LABEL organization="AdvancingTechnology Laboratory (@dvancingTech) at CSUN"
LABEL version="0.1"
LABEL description="The image contains the SCGI launch system"
LABEL maintainer="Steven.Fitzgerald@csun.edu"

RUN mkdir /scgi-daemon
ENV PATH="/scgi-daemon:$PATH"


COPY scgi-launch.bash	       /scgi-daemon/scgi-launch
COPY scgi2env-exec	       /scgi-daemon/
COPY TestingCode/emit-env.cgi  /scgi-daemon/

#ENTRYPOINT /scgi-daemon/scgi-launch localhost 8080 /scgi-daemon/cgi-program

EXPOSE 8080/tcp
