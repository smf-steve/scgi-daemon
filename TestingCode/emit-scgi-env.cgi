#! /bin/bash

# The smallest SCGI specific request
#
# METHOD URI?QUERY PROTOCOL
# server: HOST
# <other request headers>
# <blank line>
# <optional body>


# Emit response headers
echo "Status: 200 Okay"
echo "X-function: Emiting Environment variables"
echo "Content-type: text/plain"


# Emit blank line
echo ""

# Emit body
echo "# CGI Defined Environment Variables"
echo ""
echo "CONTENT_LENGTH:"  ${CONTENT_LENGTH} 
echo "SCGI:"  ${SCGI}
echo "REQUEST_METHOD:"  ${REQUEST_METHOD} 
echo "REQUEST_URI:"  ${REQUEST_URI} 
echo
echo "Request Body:"

# Echo the HTTP request body
while read _line ; do
  echo ${_line} 
done 
echo $_line

exit 0


