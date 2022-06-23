# docker-php7-apache-saxonhe

A Docker image containing Saxon-HE compiled as a PHP extension.

The [source code for Saxon-HE/C](https://www.saxonica.com/saxon-c/index.xml) is licensed under the Mozilla Public License Version 2.0.

The [SaxonC PHP API documentation](https://www.saxonica.com/saxon-c/documentation11/#!api/saxon_c_php_api) contains details of the PHP API and examples of usage.

Run `docker buildx build . --pull --load --platform linux/amd64 --tag php7-apache-saxonhe` to build the Docker image.
