#!/bin/bash

ADDRESS="169.254.216.11"
PORT="9221"

if [ $# -lt 1 ]
then
    echo "*IDN?" | ncat $ADDRESS $PORT
    echo "Interactive prompt:"
    ncat $ADDRESS $PORT
else
    cat $1 | ncat $ADDRESS $PORT
fi

exit
