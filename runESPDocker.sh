#/bin/bash

ESPCONTAINERVERSION=`cat espcontainerver`

docker run --rm -it -v $PWD:/esp/project darknet8-esp:$ESPCONTAINERVERSION 
