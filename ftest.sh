#!/usr/bin/env bash
set -e

path="flag_tests"
[[ -n $1 ]] && path+="/$1"
[[ -n $2 ]] && path+="/$2"

make flag_test
./flag_test --run_test="$path" --log_level=message

