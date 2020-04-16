#!/bin/sh
set -x
for env in ${DISTROS-ubuntu debian fedora};
do
    docker build ./${env}/ -t linpac-build-${env}:latest 2>&1 | tee docker-${env}.log
    docker run --rm -v $(cd ../../..;pwd):/src -w /src linpac-build-${env}:latest sh -c 'autoreconf --install && ./configure && make -k clean && make && make check && make install' 2>&1 | tee build-${env}.log
done
