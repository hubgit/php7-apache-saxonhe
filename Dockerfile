FROM php:7-apache-bullseye

ARG DEBIAN_FRONTEND=noninteractive

ARG jdk='openjdk-11-jdk-headless'
ARG jvm='/usr/lib/jvm/java-11-openjdk-amd64'

# needed for openjdk-11-jdk-headless
RUN mkdir -p /usr/share/man/man1

## dependencies
RUN apt-get update
RUN apt-get install -y --no-install-recommends ${jdk} unzip libxml-commons-resolver1.1-java

# edit this to use a different version of Saxon
ARG saxon='11.4'

WORKDIR /opt

## fetch
RUN curl https://www.saxonica.com/download/libsaxon-HEC-setup64-v${saxon}.zip --output saxon.zip
RUN unzip saxon.zip -d saxon

#ENV SAXONC_HOME=/usr/lib

## prepare
RUN cp saxon/libsaxon-HEC-${saxon}/libsaxonhec.so /usr/lib/
RUN cp -r saxon/libsaxon-HEC-${saxon}/rt /usr/lib/
RUN cp -r saxon/libsaxon-HEC-${saxon}/saxon-data /usr/lib/

## build
WORKDIR /opt/saxon/libsaxon-HEC-${saxon}/Saxon.C.API
RUN phpize
RUN ./configure --enable-saxon CPPFLAGS="-I${jvm}/include -I${jvm}/include/linux"
RUN make
RUN ls -al
RUN make install
RUN echo 'extension=saxon.so' > "$PHP_INI_DIR/conf.d/saxon.ini"
RUN mv "$PHP_INI_DIR/php.ini-production" "$PHP_INI_DIR/php.ini"

#RUN apt-get update \
#    ## dependencies
#    && apt-get install -y --no-install-recommends ${jdk} unzip libxml-commons-resolver1.1-java \
#    ## fetch
#    && curl https://www.saxonica.com/download/${saxon}.zip --output saxon.zip \
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
