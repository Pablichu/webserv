#!/bin/bash

# Need to run "make && make start" before executing this test script

# dirname removes "/filename.extensension" from path
RELATIVE_SCRIPT_PATH=$(dirname $BASH_SOURCE)
TARGET_FILE_NAME="tmp_hello.txt"
TARGET_FILE_PATH="${RELATIVE_SCRIPT_PATH}/www/localhost/media/${TARGET_FILE_NAME}"
REQUEST_URI="http://localhost:8080/gallery/${TARGET_FILE_NAME}"
REQUEST_EXIT_CODE=""

# -f makes curl return an error for response codes >= 400
echo "hello" > $TARGET_FILE_PATH \
  && curl -f -X DELETE $REQUEST_URI

REQUEST_EXIT_CODE="$?"

# -f checks if file exists
if [ "$REQUEST_EXIT_CODE" -eq "0" ] && [ ! -f "$TARGET_FILE_PATH" ]
then
  echo -e "\n\nDELETE request successful"
else
  #remove target file if it still exists
  if [ -f "$TARGET_FILE_PATH" ]
  then
    rm "$TARGET_FILE_PATH"
  fi
  echo -e "\nDELETE request to $REQUEST_URI failed."
  exit $REQUEST_EXIT_CODE
fi
