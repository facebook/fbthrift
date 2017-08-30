#!/bin/bash

set -e

for x in */**.mustache; do 
  ruby ./validate.rb "$x";
done
