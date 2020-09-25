FROM ubuntu

RUN apt-get clean && apt-get autoremove
RUN apt-get update && apt-get install socket

LABEL project=launch-scgi
LABEL organization="AdvancingTechnology Laboratory (@dvancingTech) at CSUN"
LABEL image="SCGI Launch"
LABEL version="0.1"
LABEL description="The image contains the SCGI launch system"
LABEL maintainer="Steven.Fitzgerald@csun.edu"

RUN mkdir /scgi-launch
ENV PATH="/scgi-launch:$PATH"


COPY scgi-launch.bash	       /scgi-launch/scgi-launch
COPY scgi2env-exec	       /scgi-launch/
COPY TestingCode/emit-env.cgi  /scgi-launch/

ENTRYPOINT /scgi-launch/scgi-launch localhost 8080 /scgi-launch/emit-env.cgi
EXPOSE 8080/tcp
