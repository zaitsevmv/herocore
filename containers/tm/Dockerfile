FROM alpine:latest

WORKDIR /tm-server

COPY tm/run.sh .
COPY thirdparty/tm .

ENTRYPOINT ["/tm-server/run.sh"]