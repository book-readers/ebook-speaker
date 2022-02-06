#!/bin/bash

if [ `whoami` != "root" ]; then
  echo "To install eBook-speaker this script needs to be executed whith root privillages."
  exit
fi

if [ $# == 1 ]; then
   PREFIX="$1"
else
   PREFIX="/usr/local/"
fi

# Compile from source
make clean
make

# Install eBook-speaker
install -s -D eBook-speaker  ${PREFIX}/bin/eBook-speaker

# generate manpage
txt2man -p eBook-speaker.txt > eBook-speaker.1
man2html eBook-speaker.1 > eBook-speaker.html.temp
tail -n +3 eBook-speaker.html.temp > eBook-speaker.html
rm -f eBook-speaker.html.temp
install -D eBook-speaker.1 ${PREFIX}/share/man/man1/eBook-speaker.1

# store .mp3 and other files
install -d ${PREFIX}/share/eBook-speaker/
cp -r COPYING Changelog License Readme TODO eBook-speaker.desktop eBook-speaker.html eBook-speaker.menu eBook-speaker.txt error.wav icons/ ${PREFIX}/share/eBook-speaker/

# create .mo files
# de for german
install -d ${PREFIX}/share/locale/de/LC_MESSAGES
msgfmt eBook-speaker.de.po -o ${PREFIX}/share/locale/de/LC_MESSAGES/eBook-speaker.mo

# nl for dutch
install -d ${PREFIX}/share/locale/nl/LC_MESSAGES
msgfmt eBook-speaker.nl.po -o ${PREFIX}/share/locale/nl/LC_MESSAGES/eBook-speaker.mo
