#!/usr/bin/bash

SERVER_PATH=tests/www
SERVER_LOCALHOST_PATH=$SERVER_PATH/localhost
PYTHON_INTERPRETER=python3

# &> is not portable, and it might be deprecated
if ! type python3 > /dev/null 2>&1
then
  PYTHON_INTERPRETER=python
fi

if [ -d $SERVER_LOCALHOST_PATH/media ]
then
  rm $SERVER_LOCALHOST_PATH/media/*
fi

PYTHON_INTERPRETER_PATH=$(command -v $PYTHON_INTERPRETER)
PERL_INTERPRETER_PATH=$(command -v perl)

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

cat << EOF > $SERVER_LOCALHOST_PATH/cgi-bin/reply.py

import sys
import os

print("Content-Type: text/plain; charset=utf-8")
print("")
print(sys.stdin.read())
for k, v in os.environ.items():
  print(k, v)
EOF

chmod u+x $SERVER_LOCALHOST_PATH/cgi-bin/reply.py

cat << EOF > $SERVER_LOCALHOST_PATH/cgi-bin/redir.py

print("Location: http://localhost:8080/gallery")
print("")
EOF

chmod u+x $SERVER_LOCALHOST_PATH/cgi-bin/redir.py

cat << EOF > $SERVER_LOCALHOST_PATH/cgi-bin/local_redir.py

print("Location: /index.html")
print("")
EOF

chmod u+x $SERVER_LOCALHOST_PATH/cgi-bin/local_redir.py

sed s:WEBSERV_PATH:$PWD:g tests/example_config.json \
	> tests/tmp_config.json

# sed -i is not portable
sed s:PYTHON_PATH:$PYTHON_INTERPRETER_PATH:g tests/tmp_config.json \
  > tests/inter_config.json

# Replace content of tmp_config with that of inter_config
cp -f tests/inter_config.json tests/tmp_config.json
rm tests/inter_config.json

sed s:PERL_PATH:$PERL_INTERPRETER_PATH:g tests/tmp_config.json \
  > tests/inter_config.json

cp -f tests/inter_config.json tests/tmp_config.json
rm tests/inter_config.json
