#!/usr/bin/env bash
set -e

path="flag_tests"
[[ -n $1 ]] && path+="/$1"
[[ -n $2 ]] && path+="/$2"

make flag_test
# ./flag_test --run_test="$path" --log_level=message --report_level=detailed 2>&1 >/dev/null | grep -A 4 'Test case.*failed' --color=always
# ./flag_test --run_test="$path" --log_level=message
./flag_test --run_test="$path" --log_level=test_suite

