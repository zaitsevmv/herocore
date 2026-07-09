FROM ubuntu:16.04

ENV LANG=C.UTF-8 LC_ALL=C.UTF-8
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    wget \
    pkg-config \
    autoconf \
    bison \
    re2c \
    libxml2-dev \
    libssl-dev \
    libcurl4-openssl-dev \
    libjpeg-dev \
    libpng-dev \
    libfreetype6-dev \
    libmcrypt-dev \
    libreadline-dev \
    libzip-dev \
    libbz2-dev \
    libxslt1-dev \
    libsqlite3-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src
RUN wget https://www.php.net/distributions/php-5.6.40.tar.gz \
    && tar -xzf php-5.6.40.tar.gz

WORKDIR /usr/src/php-5.6.40

RUN ./configure \
    --prefix=/usr/local/php5.6 \
    --with-config-file-path=/usr/local/php5.6/etc \
    --enable-mbstring \
    --enable-bcmath \
    --enable-calendar \
    --enable-exif \
    --enable-ftp \
    --enable-sockets \
    --with-openssl \
    --with-curl \
    --with-zlib \
    --with-bz2 \
    --with-jpeg-dir \
    --with-png-dir \
    --with-freetype-dir \
    --with-gd \
    --with-mcrypt \
    --with-mysql=mysqlnd \
    --with-pdo-mysql \
    --with-mysqli \
    --with-readline \
    --with-xsl \
    --with-pdo-sqlite \
    --with-sqlite3

RUN make -j$(nproc)
RUN make install

RUN ln -s /usr/local/php5.6/bin/php /usr/local/bin/php

RUN php -v

WORKDIR /xaseco

COPY thirdparty/xaseco .

ARG SERVER_NAME
ENV SERVER_NAME=${SERVER_NAME}

COPY xaseco/run.sh .

COPY get_secret.sh .
COPY .env "${SERVER_NAME}-secrets/"

ENTRYPOINT "/xaseco/run.sh" ${SERVER_NAME}