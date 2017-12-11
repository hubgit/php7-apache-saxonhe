FROM php:7-apache

ARG DEBIAN_FRONTEND=noninteractive

RUN mkdir -p /usr/share/man/man1

RUN apt-get update && apt-get -y install \
        default-jdk-headless \
        unzip \
        wget \
    && apt-get clean

WORKDIR /tmp

# edit this to use a different version of Saxon
ARG saxon='libsaxon-HEC-setup64-v1.1.0'

RUN wget https://www.saxonica.com/saxon-c/${saxon}.zip \
    && unzip ${saxon}.zip -d saxon \
    && saxon/${saxon} -batch -dest /opt/saxon \
    && rm ${saxon}.zip \
    && rm -r saxon

RUN ln -s /opt/saxon/libsaxonhec64.so /usr/lib/ \
    && ln -s /opt/saxon/rt /usr/lib/ \
    && ln -s /usr/lib/jvm/java-8-openjdk-amd64/include/linux/jni_md.h /usr/lib/jvm/java-8-openjdk-amd64/include/

WORKDIR /opt/saxon/Saxon.C.API

RUN phpize \
    && ./configure --enable-saxon CPPFLAGS="-I/usr/lib/jvm/java-8-openjdk-amd64/include" \
    && make \
    && make install \
    && echo 'extension=saxon.so' > /usr/local/etc/php/conf.d/saxon.ini
