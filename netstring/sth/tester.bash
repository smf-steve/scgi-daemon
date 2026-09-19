#! /bin/bash

FILE=${1}

../netstring -e < ${FILE} > encoded_${FILE}
../netstring -d < encoded_${FILE} > decoded_${FILE}
diff --brief  ${FILE} decoded_${FILE}
