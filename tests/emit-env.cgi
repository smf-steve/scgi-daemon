#! /bin/bash

# Basic Request via the HTTP Protocol
#
# METHOD URI?QUERY PROTOCOL
# server: HOST
# <other request headers>
# <blank line>
# <optional body>

# Basic CGI Response

# Emit response headers
echo "X-function: Emitting Environment variables"
echo "Content-type: text/plain"


# Emit blank line
echo ""

# Emit body
echo "# Web Server Defined Environment Variables"
echo
echo "HOSTNAME=$HOSTNAME"
echo "HTTPS=$HTTPS"
echo "HTTP_ACCEPT=$HTTP_ACCEPT"
echo "HTTP_ACCEPT_ENCODING=$HTTP_ACCEPT_ENCODING"
echo "HTTP_ACCEPT_LANGUAGE=$HTTP_ACCEPT_LANGUAGE"
echo "HTTP_CONNECTION=$HTTP_CONNECTION"
echo "HTTP_VIA=$HTTP_VIA"
echo "REMOTE_ADDR=$REMOTE_ADDR"
echo "REMOTE_PORT=$REMOTE_PORT"
echo "SERVER_ADDR=$SERVER_ADDR"
echo "SERVER_ADMIN=$SERVER_ADMIN"
echo "SERVER_NAME=$SERVER_NAME"
echo "SERVER_PORT=$SERVER_PORT"
echo "SERVER_PROTOCOL=$SERVER_PROTOCOL"
echo "SERVER_SIGNATURE=$SERVER_SIGNATURE"
echo "SERVER_SOFTWARE=$SERVER_SOFTWARE"
echo "@=${@}"
echo ""
echo ""
echo "# CGI Defined Environment Variables"
echo ""
echo "CONTENT_TYPE:"  ${CONTENT_TYPE}
echo "CONTENT_LENGTH:"  ${CONTENT_LENGTH} 
echo "GATEWAY_INTERFACE:"  ${GATEWAY_INTERFACE} 
echo "HTTP_HOST:"  ${HTTP_HOST} 
echo "HTTP_USER_AGENT:"  ${HTTP_USER_AGENT} 
echo "QUERY_STRING:"  ${QUERY_STRING} 
echo "REQUEST_METHOD:"  ${REQUEST_METHOD} 
echo "REQUEST_URI:"  ${REQUEST_URI} 
echo "SERVER_PROTOCOL:"  ${SERVER_PROTOCOL} 
echo "SCRIPT_FILENAME:"  ${SCRIPT_FILENAME} 
echo "SCRIPT_NAME:"  ${SCRIPT_NAME} 
echo "SERVER_NAME:"  ${SERVER_NAME} 
echo "SERVER_PORT:"  ${SERVER_PORT} 
echo "PATH_INFO:" ${PATH_INFO}
echo

