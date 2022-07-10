#!/usr/bin/python3

import sys
import os

print("HTTP/1.1 200 OK")
print("Content-type: text/plain")
print("")
print(sys.stdin.read())
for k, v in os.environ.items():
  print(k, v)
