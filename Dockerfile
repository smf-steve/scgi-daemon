FROM ubuntu
ARG PORT

RUN apt-get clean && apt-get autoremove
RUN apt-get update && apt-get install socket

LABEL project="SCGI Daemon"launch-scgi
LABEL organization="AdvancingTechnology Laboratory (@dvancingTech) at CSUN"
LABEL version="0.1"
LABEL description="The image contains the SCGI launch system"
LABEL maintainer="Steven.Fitzgerald@csun.edu"

WORKDIR /scgi-daemon
ENV PATH="/scgi-daemon:$PATH"


COPY scgi-launch.bash	       /scgi-daemon/scgi-launch
COPY scgi2env-exec	       /scgi-daemon/

# For now, we use a 'docker cp' command on the CLI to put the cgi-program in place.
# In the future, we should either
#  - use a docker build arg to pass in the name of the file. But this has be done via a URL
#      ADD  ${PROGRAM}		       /scgi-daemon/cgi-program
#  - use a MULTI-stage build to bring not only the program but also all of its stuff.
# Perhaps the multi-stage build is not used here, but in the final build process.


EXPOSE ${PORT}
CMD /scgi-daemon/scgi-launch localhost ${PORT} /scgi-daemon/cgi-program

