#! /bin/bash

FILE=${1}

echo "encoding: " ; ../netstring -e < ${FILE} > encoded_${FILE}
echo "decoding: " ; ../netstring -d < encoded_${FILE} > decoded_${FILE}
echo "diff: "     ; diff --brief  ${FILE} decoded_${FILE}
