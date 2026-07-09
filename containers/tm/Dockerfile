FROM alpine:latest

WORKDIR /tm-server

COPY thirdparty/tm .

ARG SERVER_NAME
ENV SERVER_NAME=${SERVER_NAME}

COPY get_secret.sh .
COPY tm/run.sh .

COPY .env "${SERVER_NAME}-secrets/"

ENTRYPOINT /tm-server/run.sh ${SERVER_NAME}
