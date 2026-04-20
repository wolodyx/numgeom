#!/bin/bash
script_dir=$(dirname "$(realpath "$0")")
export LD_LIBRARY_PATH=${script_dir}/../lib
${script_dir}/app-qt -platform xcb
