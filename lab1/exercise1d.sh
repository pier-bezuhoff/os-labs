#!/bin/bash
for filename in c*; do
	if [ -f $filename ]; then
		cp $filename copy-${filename#c}
	fi
done
