#!/bin/bash

SPEC=$1

SOURCES=`grep -i "^SOURCE\d\+\s*:\s*\([fht]\+tp://.*\)" ${SPEC} | sed -e 's/^.*:[[:space:]]//'`  
for URL in ${SOURCES} ; do
  curl -OL ${URL}
done

