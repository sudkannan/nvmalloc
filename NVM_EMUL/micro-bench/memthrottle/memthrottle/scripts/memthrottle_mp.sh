#!/bin/bash

usage()
{
    cat <<EOF
usage: $0 <threads per socket> <MB/s per thread> options

OPTIONS:
  -f <location of memthrottle>      Specify location of memthrottle
  -h                                Show this message
EOF
    exit
}

if [ $# -lt 2 ]
then
    usage
fi

memthrottle=/home/limkev/balance/tools/memthrottle/memthrottle/memthrottle
threads=$1
bw=$2

while getopts "hf:" OPTION
do
    case $OPTION in
	h)
	    usage
	    ;;
	f)
	    memthrottle=$OPTARG
	    ;;
	?)
	    usage
	    ;;
    esac
done

nodes=`numactl -H | grep "available" | awk '{print $2}'`

if [ $nodes -eq 1 ]
then
    $memthrottle $threads $bw
else
    i=0
    while [ $i -lt $nodes ]
    do
	numactl --cpunodebind $i --localalloc $memthrottle $threads $bw $i &
	(( i++ ))
    done
fi
