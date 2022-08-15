#!/bin/bash

nc -z localhost 8080

if [ "$?" -ne "0" ]
then
  echo "Run make && make start before running this test."
  exit 1
fi

while [ true ]
do
  curl localhost:8080 &
  curl localhost:9000/reply.cgi &
  curl localhost:3000 &
  curl localhost:8080/gallery &
  curl localhost:9000 &
  curl -i localhost:3000/oldgallery &
  curl localhost:8080/gallery/gallery.html &
  curl localhost:3000/nonexistent/path &
  curl localhost:9000/nonexistent.cgi &
  curl -X DELETE localhost:3000/invalidmethod.txt &
  curl localhost:3000/gallery &
  curl -X GET localhost:9000/reply.cgi -H "Content-Type: text/plain" \
        -d "Hello Everyone!"
  # There is no need to call wait for this use case
  sleep 0.09 #seconds
done
