#!/bin/sh

for i in `seq 0 100`
do
    choice=$(shuf -i 0-1 -n 1)
    if [ $choice -eq 1 ]
    then
        from=/dev/urandom
        to=/dev/mapper/proxy
    else
        from=/dev/mapper/proxy
        to=/dev/null
    fi
    bss=$(shuf -i 1-20 -n 1)k
    cnt=$(shuf -i 1-30 -n 1)
    sudo dd if=$from of=$to bs=$bss count=$cnt
done
