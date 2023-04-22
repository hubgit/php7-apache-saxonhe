FROM --platform=linux/amd64 php:7-apache-bullseye

ARG DEBIAN_FRONTEND=noninteractive

ARG jdk='openjdk-11-jdk-headless'
ARG jvm='/usr/lib/jvm/java-11-openjdk-amd64'

# needed for openjdk-11-jdk-headless
RUN mkdir -p /usr/share/man/man1

## dependencies
RUN apt-get update && apt-get install -y --no-install-recommends ${jdk} unzip libxml-commons-resolver1.1-java

# edit this to use a different version of Saxon
ARG saxon='12.1'

WORKDIR /opt

## fetch
RUN curl https://www.saxonica.com/download/libsaxon-HEC-linux-v${saxon}.zip --output saxon.zip
RUN unzip saxon.zip -d saxon

#ENV SAXONC_HOME=/usr/lib

## prepare
RUN cp saxon/libsaxon-HEC-linux-v${saxon}/libs/nix/libsaxon-hec-${saxon}.so /usr/lib/

## build
WORKDIR /opt/saxon/libsaxon-HEC-linux-v${saxon}/Saxon.C.API
RUN phpize
RUN ./configure --enable-saxon
RUN make
RUN ls -al
RUN make install
RUN echo 'extension=saxon.so' > "$PHP_INI_DIR/conf.d/saxon.ini"
RUN mv "$PHP_INI_DIR/php.ini-production" "$PHP_INI_DIR/php.ini"

## install a newer version of libc6 (needs at least 2.32)
RUN echo "deb https://ftp.debian.org/debian sid main" >> /etc/apt/sources.list
RUN apt-get update && apt-get -t sid install -y libc6 libc6-dev libc6-dbg libcrypt1
