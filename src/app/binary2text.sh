#!/bin/bash

shader_file=$1
xxd -p -c 4 ${shader_file} | awk '{printf "0x%s%s", $1, (NR % 4 == 0 ? ",\n" : ", ")}' | sed '$s/, $//' > ${shader_file}.h
