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
  rm -rf $SERVER_LOCALHOST_PATH/media
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
from signal import signal, SIGPIPE, SIG_DFL  
signal(SIGPIPE,SIG_DFL)
print("Content-Type: text/plain; charset=utf-8")
print("")
print(sys.stdin.read())
for k, v in os.environ.items():
  print(k, v)
EOF

chmod u+x $SERVER_LOCALHOST_PATH/cgi-bin/reply.py

cat << EOF > $SERVER_LOCALHOST_PATH/cgi-bin/redir.py

from signal import signal, SIGPIPE, SIG_DFL  
signal(SIGPIPE,SIG_DFL)
print("Location: http://localhost:8080/gallery")
print("")
EOF

chmod u+x $SERVER_LOCALHOST_PATH/cgi-bin/redir.py

cat << EOF > $SERVER_LOCALHOST_PATH/cgi-bin/local_redir.py

from signal import signal, SIGPIPE, SIG_DFL  
signal(SIGPIPE,SIG_DFL)
print("Location: /index.html")
print("")
EOF

chmod u+x $SERVER_LOCALHOST_PATH/cgi-bin/local_redir.py

cat << EOF > $SERVER_LOCALHOST_PATH/cgi-bin/query.pl

use strict;
use warnings;
use CGI;

# Inserting '\' before dollar sign to prevent variable substitution from bash
my \$cgi = CGI->new;
# Array of query keys
my @q_keys;

# Subroutine to add an HTML paragraph
sub addParagraph {
  print "<p>";
  # Extracts the first item off the list of arguments and prints it
  print shift;
  print "</p>";
}
# Prints Content-Type: text/html
print \$cgi->header;
# Opens html and body tags and sets the argument as title
print \$cgi->start_html("Query Parameters");
# Inserts query keys into array
@q_keys = \$cgi->param();
# Checks if array is empty
if (@q_keys == 0) {
  addParagraph("Include query parameters in the url.");
  addParagraph("Example: localhost:9000/query.pl?hola=mundo&hello=world");
}
else {
  my \$key;
  my \$value;

  foreach \$key (@q_keys) {
    \$value = \$cgi->param(\$key);
    addParagraph("\$key = \$value");
  }
}
# Closes html and body tags
print \$cgi->end_html;
EOF

chmod u+x $SERVER_LOCALHOST_PATH/cgi-bin/query.pl

cat << EOF > $SERVER_LOCALHOST_PATH/cgi-bin/form.pl

use strict;
use warnings;
use CGI;

# Inserting '\' before dollar sign to prevent variable substitution from bash
my \$cgi = CGI->new;
# Array of form keys
my @f_keys;

# Subroutine to add an HTML paragraph
sub addParagraph {
  print "<p>";
  # Extracts the first item off the list of arguments and prints it
  print shift;
  print "</p>";
}
# Prints Content-Type: text/html
print \$cgi->header;
# Opens html and body tags and sets the argument as title
print \$cgi->start_html("Form Data");
if (\$ENV{REQUEST_METHOD} eq 'GET')
{
  # Non-interpolating heredoc
  print <<'END_FORM'
  <form action="/form.pl" method="post">
    <div style="padding: 20px 0">
      <label for="name">Name:</label>
      <input type="text" id="name" name="name">
    </div>
    <div style="padding: 20px 0">
      <label for="mail">Email:</label>
      <input type="email" id="mail" name="email">
    </div>
    <div style="padding: 20px 0">
      <label for="msg">Message:</label>
      <textarea id="msg" name="message" style="vertical-align: top"></textarea>
    </div>
    <div style="padding: 20px 0">
      <button type="submit">Submit</button>
    </div>
  </form>
END_FORM
}
else
{
  # Inserts form keys into array
  @f_keys = \$cgi->param();
  # Checks if array is empty
  if (@f_keys == 0) {
    addParagraph("Received an empty form.");
  }
  else {
    my \$key;
    my \$value;

    foreach \$key (@f_keys) {
      \$value = \$cgi->param(\$key);
      addParagraph("\$key = \$value");
    }
  }
}
# Closes html and body tags
print \$cgi->end_html;
EOF

chmod u+x $SERVER_LOCALHOST_PATH/cgi-bin/form.pl

cat << EOF > $SERVER_LOCALHOST_PATH/cgi-bin/cookie.pl

use CGI qw/:standard/;
use CGI::Cookie;
 
# Create new cookies and send them
\$cookie1 = CGI::Cookie->new(-name=>'HIPPIE_COOKIE',-value=>42);
#\$cookie2 = CGI::Cookie->new(-name=>'IMAGINE_DRAGONS_IS_OVERRATED',-value=>666);
print header(-cookie=>[\$cookie1,\$cookie2]);
 
# fetch existing cookies
%cookies = CGI::Cookie->fetch;
\$hippieCookie = \$cookies{'HIPPIE_COOKIE'}->value;
#\$hippieCookie2 = \$cookies{'IMAGINE_DRAGONS_IS_OVERRATED'}->value;
if (\$hippieCookie)
{
  print "Found hippie cookie that says -> \$hippieCookie";
  #print "Capatapa -> \$hippieCookie2";
}
EOF

chmod u+x $SERVER_LOCALHOST_PATH/cgi-bin/cookie.pl

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
