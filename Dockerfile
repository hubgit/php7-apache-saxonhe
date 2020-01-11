FROM php:7-apache-buster

ARG DEBIAN_FRONTEND=noninteractive

# edit this to use a different version of Saxon
ARG saxon='libsaxon-HEC-setup64-v1.2.1'

ARG jdk='openjdk-11-jdk-headless'

ARG jvm='/usr/lib/jvm/java-11-openjdk-amd64'

# needed for openjdk-11-jdk-headless
RUN mkdir -p /usr/share/man/man1

## dependencies
RUN apt-get update
RUN apt-get install -y --no-install-recommends ${jdk} unzip libxml-commons-resolver1.1-java

## fetch
RUN curl https://www.saxonica.com/saxon-c/${saxon}.zip --output saxon.zip
RUN unzip saxon.zip -d saxon

## install
RUN saxon/${saxon} -batch -dest /opt/saxon

## patch
COPY php7_saxon.cpp /opt/saxon/Saxon.C.API/

## prepare
RUN ln -s /opt/saxon/libsaxonhec.so /usr/lib/
RUN ln -s /opt/saxon/rt /usr/lib/

## build
WORKDIR /opt/saxon/Saxon.C.API
RUN phpize
RUN ./configure --enable-saxon CPPFLAGS="-I${jvm}/include -I${jvm}/include/linux"
RUN make
RUN make install
RUN echo 'extension=saxon.so' > "$PHP_INI_DIR/conf.d/saxon.ini"
RUN mv "$PHP_INI_DIR/php.ini-production" "$PHP_INI_DIR/php.ini"

#RUN apt-get update \
#    ## dependencies
#    && apt-get install -y --no-install-recommends ${jdk} unzip libxml-commons-resolver1.1-java \
#    ## fetch
#    && curl https://www.saxonica.com/saxon-c/${saxon}.zip --output saxon.zip \
#    && unzip saxon.zip -d saxon \
#    && rm saxon.zip \
#    ## install
#    && saxon/${saxon} -batch -dest /opt/saxon \
#    && rm -r saxon \
#    ## prepare
#    && ln -s /opt/saxon/libsaxonhec.so /usr/lib/ \
#    && ln -s /opt/saxon/rt /usr/lib/ \
#    ## build
#    && cd /opt/saxon/Saxon.C.API \
#    && phpize \
#    && ./configure --enable-saxon CPPFLAGS="-I${jvm}/include -I${jvm}/include/linux" \
#    && make \
#    && make install \
#    && echo 'extension=saxon.so' > "$PHP_INI_DIR/conf.d/saxon.ini" \
#    && rm -r /opt/saxon/Saxon.C.API \
#    && mv "$PHP_INI_DIR/php.ini-production" "$PHP_INI_DIR/php.ini" \
#    ## clean
#    && apt-get clean \
#    && apt-get remove -y ${jdk} unzip \
#    && apt-get autoremove -y \
#    && rm -rf /var/lib/apt/lists/
