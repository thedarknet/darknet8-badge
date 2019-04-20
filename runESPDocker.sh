#/bin/bash
set -e

ESPCONTAINERVERSION=`cat docker/espcontainerver`

docker run --name dndev --rm -it --privileged -e USER_ID=$UID -e GROUP_ID=`id -g` -v /dev/bus/usb:/dev/bus/usb -v $PWD:/esp/project -v /tmp:/ccache darknet8-esp:$ESPCONTAINERVERSION 
#docker run --name dndev --user $UID:`id -g` --rm -it --privileged -e USER_ID=$UID -e GROUP_ID=`id -g` -v /dev/bus/usb:/dev/bus/usb -v $PWD:/esp/project darknet8-esp:$ESPCONTAINERVERSION 
