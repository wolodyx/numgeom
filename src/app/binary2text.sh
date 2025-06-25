#!/bin/bash

shader_file=$1
xxd -p -c4 ${shader_file} | awk '{
    split($1, bytes, "");
    printf "0x%s%s%s%s%s",
           bytes[7] bytes[8],
           bytes[5] bytes[6],
           bytes[3] bytes[4],
           bytes[1] bytes[2],
           (NR % 4 == 0 ? ",\n" : ", ")
}' >> ${shader_file}.h
