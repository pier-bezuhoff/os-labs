#!/bin/bash
# should be executed with sudo:)
FILE=/etc/shadow
read sequence
grep $sequence $FILE
