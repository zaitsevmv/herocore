FROM ubuntu:18.04

ENV LANG=C.UTF-8 LC_ALL=C.UTF-8
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y php php-xml php-mysql php-mbstring && \
    rm -rf /var/lib/apt/lists/*

RUN php -v

WORKDIR /xaseco

COPY thirdparty/xaseco .

ARG SERVER_NAME
ENV SERVER_NAME=${SERVER_NAME}

COPY xaseco/run.sh .

COPY get_secret.sh .
COPY .env "${SERVER_NAME}-secrets/"

ENTRYPOINT "/xaseco/run.sh" ${SERVER_NAME}