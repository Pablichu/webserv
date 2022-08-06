#!/bin/bash

# Need to run "make && make start" before executing this test script

# dirname removes "/filename.extensension" from path
RELATIVE_SCRIPT_PATH=$(dirname $BASH_SOURCE)
TARGET_FILE_NAME="tmp_hello.txt"
TARGET_FILE_PATH="${RELATIVE_SCRIPT_PATH}/www/localhost/media/${TARGET_FILE_NAME}"
MEDIA_SERVER="http://localhost:8080"
CGI_SERVER="http://localhost:9000"
MEDIA_REQUEST_URI="${MEDIA_SERVER}/gallery/${TARGET_FILE_NAME}"
CGI_REQUEST_URI="${CGI_SERVER}/${TARGET_FILE_NAME}"
REQUEST_EXIT_CODE=""



#### VALID FILE DELETION ####

# -f makes curl return an error for response codes >= 400
echo "hello" > $TARGET_FILE_PATH \
  && curl -f -s -X DELETE $MEDIA_REQUEST_URI > /dev/null

REQUEST_EXIT_CODE="$?"

# -f checks if file exists
if [ "$REQUEST_EXIT_CODE" -ne "0" ] || [ -f "$TARGET_FILE_PATH" ]
then
  #remove target file if it still exists
  if [ -f "$TARGET_FILE_PATH" ]
  then
    rm "$TARGET_FILE_PATH"
  fi
  if [ "$REQUEST_EXIT_CODE" -ne "22" ]
  then
    echo -e "\nRun 'make && make start' before executing this test script"
  else
    echo -e "\nVALID FILE DELETION TEST failed."
    echo -e "\nDELETE request to $MEDIA_REQUEST_URI failed."
  fi
  exit $REQUEST_EXIT_CODE
fi


#### NOT FOUND FILE ####

curl -f -s -X DELETE $MEDIA_REQUEST_URI

REQUEST_EXIT_CODE="$?"

if [ "$REQUEST_EXIT_CODE" -eq "0" ]
then
  echo -e "\nNOT FOUND FILE TEST failed."
  echo -e "\nDELETE request to $MEDIA_REQUEST_URI must fail."
  exit 1
fi


#### FORBIDDEN METHOD ####

curl -f -s -X DELETE $CGI_REQUEST_URI

REQUEST_EXIT_CODE="$?"

if [ "$REQUEST_EXIT_CODE" -ne "0" ]
then
  echo -e "\nFORBIDDEN METHOD TEST failed."
  echo -e "\nDELETE request to ${CGI_REQUEST_URI} must fail."
  exit 1
fi

echo -e "\nDELETE tests OK."
