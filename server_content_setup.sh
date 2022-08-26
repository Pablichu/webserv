#!/usr/bin/bash

SERVER_PATH=tests/www
SERVER_LOCALHOST_PATH=$SERVER_PATH/localhost
PYTHON_INTERPRETER=python3

if ! type python3 > /dev/null
then
  PYTHON_INTERPRETER=python
fi

if [ -d $SERVER_LOCALHOST_PATH/media ]
then
  rm $SERVER_LOCALHOST_PATH/media/*
fi

mkdir -p $SERVER_LOCALHOST_PATH
mkdir -p $SERVER_LOCALHOST_PATH/gallery
mkdir -p $SERVER_LOCALHOST_PATH/media
mkdir -p $SERVER_LOCALHOST_PATH/cgi-bin

cat << EOF > $SERVER_LOCALHOST_PATH/index.html
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8">
    <title>INDEX</title>
    <meta name="viewport" content="width=device-width,initial-scale=1">
  </head>
  <body>
    <h1>INDEX</h1>
  </body>
</html>
EOF

sed s/INDEX/GALLERY/g $SERVER_LOCALHOST_PATH/index.html \
  > $SERVER_LOCALHOST_PATH/gallery/gallery.html

sed s/INDEX/NOT_FOUND/g $SERVER_LOCALHOST_PATH/index.html \
  > $SERVER_PATH/404.html

cat << EOF > $SERVER_LOCALHOST_PATH/cgi-bin/reply.cgi
#!/usr/bin/$PYTHON_INTERPRETER

import sys
import os

print("Content-Type: text/plain; charset=utf-8")
print("")
print(sys.stdin.read())
for k, v in os.environ.items():
  print(k, v)
EOF

chmod u+x $SERVER_LOCALHOST_PATH/cgi-bin/reply.cgi
