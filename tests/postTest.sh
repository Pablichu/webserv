#!/bin/bash

# Need to run "make && make start" before executing this test script

# dirname removes "/filename.extensension" from path
RELATIVE_SCRIPT_PATH=$(dirname $BASH_SOURCE)
UPLOAD_FILE_PATH="${RELATIVE_SCRIPT_PATH}/../sources/Config.cpp"
TARGET_FILE_NAME="post_test.txt"
TARGET_FILE_PATH="${RELATIVE_SCRIPT_PATH}/www/localhost/media/${TARGET_FILE_NAME}"
SERVER="http://localhost:8080"
REQUEST_URI="${SERVER}/gallery/media/${TARGET_FILE_NAME}"
LOG_FILE_NAME="webserv_post.txt"
LOG_FILE_PATH="/tmp/${LOG_FILE_NAME}"
EXIT_CODE=""

# POST big text file

# -f makes curl return an error for response codes >= 400
#
# &> is not portable. 2>&1 redirects stderr to stdout
#
curl -v -f --data-binary @"${UPLOAD_FILE_PATH}" $REQUEST_URI \
  > $LOG_FILE_PATH 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ] || [ ! -f "$TARGET_FILE_PATH" ]
then
  if [ "$EXIT_CODE" -ne "22" ]; then
    echo "Run make && make start before executing this test script."
  else
    echo "First POST request failed."; fi
  if [ -f "$TARGET_FILE_PATH" ]; then rm $TARGET_FILE_PATH; fi
  exit $EXIT_CODE
fi

cat $LOG_FILE_PATH | grep "HTTP/1.1 201 Created" > /dev/null 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ]
then
  echo "First POST request status code incorrect."
  if [ -f "$TARGET_FILE_PATH" ]; then rm $TARGET_FILE_PATH; fi
  exit $EXIT_CODE
fi

diff $UPLOAD_FILE_PATH $TARGET_FILE_PATH > /dev/null 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ]
then
  echo "First POST request file contents are incorrect."
  if [ -f "$TARGET_FILE_PATH" ]; then rm $TARGET_FILE_PATH; fi
  exit $EXIT_CODE
fi

# POST to existing file to append

curl -v -f --data-binary @"${UPLOAD_FILE_PATH}" $REQUEST_URI \
  > $LOG_FILE_PATH 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ]
then
  echo "Second POST request failed."
  if [ -f "$TARGET_FILE_PATH" ]; then rm $TARGET_FILE_PATH; fi
  exit $EXIT_CODE
fi

cat $LOG_FILE_PATH | grep "HTTP/1.1 200 OK" > /dev/null 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ]
then
  echo "Second POST request status code incorrect."
  if [ -f "$TARGET_FILE_PATH" ]; then rm $TARGET_FILE_PATH; fi
  exit $EXIT_CODE
fi

# The appended file should have the content of the upload file duplicated.
# So if diff determines that files are equal, it means that the file
# was overwritten.

diff $UPLOAD_FILE_PATH $TARGET_FILE_PATH > /dev/null 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -eq "0" ]
then
  echo "Second POST request did not append."
  if [ -f "$TARGET_FILE_PATH" ]; then rm $TARGET_FILE_PATH; fi
  exit $EXIT_CODE
fi

# DELETE uploaded file

curl -f -s -X DELETE $REQUEST_URI > /dev/null 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ] || [ -f "$TARGET_FILE_PATH" ]
then
  echo "DELETE request failed."
  if [ -f "$TARGET_FILE_PATH" ]; then rm "$TARGET_FILE_PATH"; fi
  exit $EXIT_CODE
fi

echo -e "\nPOST tests passed. Congrats!\n"
