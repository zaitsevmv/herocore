FROM ubuntu:16.04

ENV LANG=C.UTF-8 LC_ALL=C.UTF-8

RUN apt-get update
RUN apt-get install -y software-properties-common
RUN add-apt-repository -y ppa:ondrej/php
RUN apt-get update
RUN apt-get install -y php5.6

WORKDIR /xaseco

COPY xaseco/run.sh .
COPY thirdparty/xaseco .

ENTRYPOINT ["/xaseco/run.sh"]