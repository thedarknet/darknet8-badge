#!/bin/bash
ESPCONTAINERVERSION=`cat espcontainerver`

echo $ESPCONTAINERVERSION

docker build -t darknet8-esp:$ESPCONTAINERVERSION .
