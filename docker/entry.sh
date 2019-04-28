#!/bin/bash
DEFAULT_UID=1000
DEFAULT_GID=1000

if [ $USER_ID = "" ]; then
	USER_ID = $DEFAULT_UID
fi

if [ $GROUP_ID = "" ]; then
	GROUP_ID = $DEFAULT_GID
fi

CURRENT_DIR=$PWD

chmod 777 /ccache
echo "Host UID: $USER_ID GID: $GROUP_ID"
groupmod -o -g $GROUP_ID dndev
usermod -o -u $USER_ID -g $GROUP_ID dndev
su dndev

