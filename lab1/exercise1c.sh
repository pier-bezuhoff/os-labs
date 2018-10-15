#!/bin/bash
for filename in b*; do
	mv $filename ${filename#b}
done
