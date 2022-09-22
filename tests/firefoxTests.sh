#!/bin/bash

# Open Firefox browser window and test all GET request functionalities
firefox http://localhost:8080 \
        http://localhost:9000/reply.py \
        http://localhost:3000/oldgallery \
        http://localhost:8080/gallery \
        http://localhost:9000 \
        http://localhost:3000/nonexistent/path \
        http://localhost:8080/gallery/gallery.html \
        http://localhost:9000/nonexistent.py \
        http://localhost:3000 \
        http://localhost:8080/nonexistent.html
