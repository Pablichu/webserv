#!/usr/bin/bash

SERVER_PATH=/tmp/www
SERVER_LOCALHOST_PATH=$SERVER_PATH/localhost

mkdir -p $SERVER_LOCALHOST_PATH
mkdir $SERVER_LOCALHOST_PATH/gallery
mkdir $SERVER_LOCALHOST_PATH/media
mkdir $SERVER_LOCALHOST_PATH/cgi-bin

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
#!/usr/bin/python3

import sys

print("HTTP/1.1 200 OK")
print("Content-type: text/plain")
print("")
print(sys.stdin.read())
EOF
