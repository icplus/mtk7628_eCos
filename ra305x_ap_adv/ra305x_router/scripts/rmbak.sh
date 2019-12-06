#!/bin/sh

for file in `find . -name *~`; do
	rm -f $file
done
