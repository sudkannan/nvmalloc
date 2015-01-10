#!/bin/zsh

. ../../gnuplot/filterplot.sh


plot_memlat_throughput()
{
    local datafile=$1
    local outfile=$2
    local mpl=$3
    #outfile=`echo $(spaces2underscores $title) | tr '[A-Z]' '[a-z]'| sed -e 's/\.//g' | sed -e 's/=//g'`.pdf

    #plotswizzling=$(selecttoplot $datafile "swizzling" "RangeMultiplier" "numberOfTransactionsPerSecond" "Expr=^swizzling" "MPL=24" "writeperc=0")
    plotnoswizzling1=$(selecttoplot $datafile "bufferpool 0.5" "EffectiveReadLatency" "numberOfTransactionsPerSecond" "Expr=^noswizzling" "MPL=$mpl" "BUFFERPOOL=4194304")
    plotnoswizzling2=$(selecttoplot $datafile "bufferpool 0.25" "EffectiveReadLatency" "numberOfTransactionsPerSecond" "Expr=^noswizzling" "MPL=$mpl" "BUFFERPOOL=2097152")
    plotnoswizzling3=$(selecttoplot $datafile "bufferpool 0.125" "EffectiveReadLatency" "numberOfTransactionsPerSecond" "Expr=^noswizzling" "MPL=$mpl" "BUFFERPOOL=1048576")
    plotnoswizzling4=$(selecttoplot $datafile "bufferpool 0.06125" "EffectiveReadLatency" "numberOfTransactionsPerSecond" "Expr=^noswizzling" "MPL=$mpl" "BUFFERPOOL=524288")
    plotmainmemory=$(selecttoplot $datafile "main-memory" "EffectiveReadLatency" "numberOfTransactionsPerSecond" "Expr=^mainmemory" "MPL=$mpl")

    plotstr=$(concatenate $plotnoswizzling1 $plotnoswizzling2 $plotnoswizzling3 $plotnoswizzling4 $plotmainmemory)
    plotmulti "$outfile" "Memory latency vs. Throughput ($mpl threads)" "Memory latency (ns)" "Throughput (qps)" $plotstr
}



for DATAFILE in $@
do 
    plot_memlat_throughput $DATAFILE "nvram-1t.pdf" 1
    plot_memlat_throughput $DATAFILE "nvram-4t.pdf" 4
done
