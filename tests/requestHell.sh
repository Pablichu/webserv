#!/bin/bash

RELATIVE_SCRIPT_PATH=$(dirname $BASH_SOURCE)
UPLOAD_TXT_FILE_PATH="${RELATIVE_SCRIPT_PATH}/../sources/Config.cpp"
SERVER="http://localhost:8080"
TXT_REQUEST_URI="${SERVER}/gallery/media"
RAND_VALUE=""
TEST_FILE_NAME=""

nc -z localhost 8080

if [ "$?" -ne "0" ]
then
  echo "Run make && make start before running this test."
  exit 1
fi

while [ true ]
do
  curl localhost:8080 &
  curl localhost:9000/reply.py &
  curl localhost:3000 &
  curl localhost:8080/gallery &
  curl localhost:9000 &
  curl -i localhost:3000/oldgallery &
  curl localhost:8080/gallery/gallery.html &
  curl localhost:3000/nonexistent/path &
  curl localhost:9000/nonexistent.py &
  curl -X DELETE localhost:3000/invalidmethod.txt &
  curl localhost:3000/gallery &
  curl -X GET localhost:9000/reply.py -H "Content-Type: text/plain" \
        -d "Hello Everyone!"
  RAND_VALUE=$(openssl rand -hex 10)
  TEST_FILE_NAME="$RAND_VALUE.txt"
  curl -f --data-binary @"${UPLOAD_TXT_FILE_PATH}" "$TXT_REQUEST_URI/$TEST_FILE_NAME" \
    -H "Content-Type: text/plain"
  curl -f -s -X DELETE "$TXT_REQUEST_URI/$TEST_FILE_NAME"
  curl -f --data-binary @"${UPLOAD_TXT_FILE_PATH}" "$TXT_REQUEST_URI/$TEST_FILE_NAME" \
    -H "Content-Type: text/plain" -H "Transfer-Encoding: chunked"
  curl -f -s -X DELETE "$TXT_REQUEST_URI/$TEST_FILE_NAME"
  # There is no need to call wait for this use case
  sleep 0.09 #seconds
done
