#!/bin/bash

# Need to run "make && make start" before executing this test script

# dirname removes "/filename.extensension" from path
RELATIVE_SCRIPT_PATH=$(dirname $BASH_SOURCE)
UPLOAD_TXT_FILE_PATH="${RELATIVE_SCRIPT_PATH}/../sources/Config.cpp"
DOWNLOAD_IMG_URI="https://ia800304.us.archive.org/7/items/SPD-SLRSY-1826/rocketry_21.jpg"
DOWNLOAD_IMG_FILE_PATH="${RELATIVE_SCRIPT_PATH}/../post_test.jpg"
TARGET_TXT_FILE_NAME="post_test.txt"
TARGET_TXT_FILE_PATH="${RELATIVE_SCRIPT_PATH}/www/localhost/media/${TARGET_TXT_FILE_NAME}"
TARGET_IMG_FILE_NAME="post_test.jpg"
TARGET_IMG_FILE_PATH="${RELATIVE_SCRIPT_PATH}/www/localhost/media/${TARGET_IMG_FILE_NAME}"
SERVER="http://localhost:8080"
TXT_REQUEST_URI="${SERVER}/gallery/media/${TARGET_TXT_FILE_NAME}"
IMG_REQUEST_URI="${SERVER}/gallery/media/${TARGET_IMG_FILE_NAME}"
LOG_FILE_NAME="webserv_post.txt"
LOG_FILE_PATH="/tmp/${LOG_FILE_NAME}"
EXIT_CODE=""

# POST big text file

# -f makes curl return an error for response codes >= 400
#
# &> is not portable. 2>&1 redirects stderr to stdout
#
curl -v -f --data-binary @"${UPLOAD_TXT_FILE_PATH}" $TXT_REQUEST_URI \
  > $LOG_FILE_PATH 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ] || [ ! -f "$TARGET_TXT_FILE_PATH" ]
then
  if [ "$EXIT_CODE" -ne "22" ]; then
    echo "Run make && make start before executing this test script."
  else
    echo "First POST request failed."; fi
  if [ -f "$TARGET_TXT_FILE_PATH" ]; then rm $TARGET_TXT_FILE_PATH; fi
  exit $EXIT_CODE
fi

cat $LOG_FILE_PATH | grep "HTTP/1.1 201 Created" > /dev/null 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ]
then
  echo "First POST request status code incorrect."
  if [ -f "$TARGET_TXT_FILE_PATH" ]; then rm $TARGET_TXT_FILE_PATH; fi
  exit $EXIT_CODE
fi

diff $UPLOAD_TXT_FILE_PATH $TARGET_TXT_FILE_PATH > /dev/null 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ]
then
  echo "First POST request file contents are incorrect."
  if [ -f "$TARGET_TXT_FILE_PATH" ]; then rm $TARGET_TXT_FILE_PATH; fi
  exit $EXIT_CODE
fi

# POST to existing file to append

curl -v -f --data-binary @"${UPLOAD_TXT_FILE_PATH}" $TXT_REQUEST_URI \
  > $LOG_FILE_PATH 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ]
then
  echo "Second POST request failed."
  if [ -f "$TARGET_TXT_FILE_PATH" ]; then rm $TARGET_TXT_FILE_PATH; fi
  exit $EXIT_CODE
fi

cat $LOG_FILE_PATH | grep "HTTP/1.1 200 OK" > /dev/null 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ]
then
  echo "Second POST request status code incorrect."
  if [ -f "$TARGET_TXT_FILE_PATH" ]; then rm $TARGET_TXT_FILE_PATH; fi
  exit $EXIT_CODE
fi

# The appended file should have the content of the upload file duplicated.
# So if diff determines that files are equal, it means that the file
# was overwritten.

diff $UPLOAD_TXT_FILE_PATH $TARGET_TXT_FILE_PATH > /dev/null 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -eq "0" ]
then
  echo "Second POST request did not append."
  if [ -f "$TARGET_TXT_FILE_PATH" ]; then rm $TARGET_TXT_FILE_PATH; fi
  exit $EXIT_CODE
fi

# DELETE uploaded file

curl -f -s -X DELETE $TXT_REQUEST_URI > /dev/null 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ] || [ -f "$TARGET_TXT_FILE_PATH" ]
then
  echo "DELETE request failed."
  if [ -f "$TARGET_TXT_FILE_PATH" ]; then rm "$TARGET_TXT_FILE_PATH"; fi
  exit $EXIT_CODE
fi

# POST big text file in chunks

curl -v -f --data-binary @"${UPLOAD_TXT_FILE_PATH}" $TXT_REQUEST_URI \
  -H "Transfer-Encoding: chunked" > $LOG_FILE_PATH 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ] || [ ! -f "$TARGET_TXT_FILE_PATH" ]
then
  echo "Chunked POST request failed."; fi
  if [ -f "$TARGET_TXT_FILE_PATH" ]; then rm $TARGET_TXT_FILE_PATH; fi
  exit $EXIT_CODE
fi

cat $LOG_FILE_PATH | grep "HTTP/1.1 201 Created" > /dev/null 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ]
then
  echo "Chunked POST request status code incorrect."
  if [ -f "$TARGET_TXT_FILE_PATH" ]; then rm $TARGET_TXT_FILE_PATH; fi
  exit $EXIT_CODE
fi

diff $UPLOAD_TXT_FILE_PATH $TARGET_TXT_FILE_PATH > /dev/null 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ]
then
  echo "Chunked POST request file contents are incorrect."
  if [ -f "$TARGET_TXT_FILE_PATH" ]; then rm $TARGET_TXT_FILE_PATH; fi
  exit $EXIT_CODE
fi

# Download image

curl -L $DOWNLOAD_IMG_URI -o $DOWNLOAD_IMG_FILE_PATH > /dev/null 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ]
then
  echo "Image download failed. Check if url is still valid."
  exit $EXIT_CODE
fi

# Upload image

curl -v -f --data-binary @"$DOWNLOAD_IMG_FILE_PATH" $IMG_REQUEST_URI \
  -H "Content-Type: image/jpeg" > $LOG_FILE_PATH 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ] || [ ! -f "$TARGET_IMG_FILE_PATH" ]
then
  echo "Image upload failed."
  rm $DOWNLOAD_IMG_FILE_PATH
  exit $EXIT_CODE
fi

cat $LOG_FILE_PATH | grep "HTTP/1.1 201 Created" > /dev/null 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ]
then
  echo "Image POST request status code incorrect."
  if [ -f "$TARGET_IMG_FILE_PATH" ]; then rm $TARGET_IMG_FILE_PATH; fi
  rm $DOWNLOAD_IMG_FILE_PATH
  exit $EXIT_CODE
fi

diff $DOWNLOAD_IMG_FILE_PATH $TARGET_IMG_FILE_PATH > /dev/null 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ]
then
  echo "Image POST file contents are incorrect."
  if [ -f "$TARGET_IMG_FILE_PATH" ]; then rm $TARGET_IMG_FILE_PATH; fi
  rm $DOWNLOAD_IMG_FILE_PATH
  exit $EXIT_CODE
fi

# DELETE uploaded image

curl -f -s -X DELETE $IMG_REQUEST_URI > /dev/null 2>&1

EXIT_CODE="$?"

if [ "$EXIT_CODE" -ne "0" ] || [ -f "$TARGET_IMG_FILE_PATH" ]
then
  echo "DELETE request failed."
  if [ -f "$TARGET_IMG_FILE_PATH" ]; then rm "$TARGET_IMG_FILE_PATH"; fi
  rm $DOWNLOAD_IMG_FILE_PATH
  exit $EXIT_CODE
fi

echo -e "\nPOST tests passed. Congrats!\n"
