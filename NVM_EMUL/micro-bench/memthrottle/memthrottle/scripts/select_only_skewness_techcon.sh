#!/bin/sh
. ../../scripts/select_common.sh

size_to_nrecords() {
    local SIZE_IN_KB=$1
    echo `echo "${SIZE_IN_KB}*1024/40/1.55" | bc -l | xargs printf "%1.0f"`
}

# <experiment> <skewness> <#threads> <integer # 2^20 records> <inquery-verify-level>
run_expr() {
    local EXPR=$1
    local SKEWNESS=$2
    local WRITEPERC=$3
    local MPL=$4
    local BUFFERPOOL_IN_KB=$5
    local NRECORDS=$6
    local RANGEMULT=$7
    local WARMUP=$8
    local EXTRAREADLATENCY=$9
    local MEMSPEED=${10}
    local NUMANODE=${11}
    local REP=${12}
    
    local NUMQUERIES_PER_THREAD=8000000
    local WARMUP="range:0:1"
    local NUMQUERIES_PER_THREAD=1000
    local NUMANODE=0

    local INQVRFYLEVEL=0

    local KEY="select_only_${EXPR}_LOG:ssd_DATAhdd_SKEWNESS${SKEWNESS}_WRITEPERC${WRITEPERC}_INSERTPERC0_DELETEPERC0_MPL${MPL}_RANGEMULT${RANGEMULT}_MEMSPEED${MEMSPEED}_NUMANODE${NUMANODE}_EXTRAREADLATENCY${EXTRAREADLATENCY}_BUFFERPOOL${BUFFERPOOL_IN_KB}"
    local OUTPUT_FNAME="${EXPERIMENT_OUTPUT_DIR}/${KEY}_REP${REP}"
    local RANGEMAX=`echo "${NRECORDS}*${RANGEMULT}" | bc -l | xargs printf "%1.0f"`
    local NUMQUERIES=`echo "${NUMQUERIES_PER_THREAD}*${MPL}" | bc -l | xargs printf "%1.0f"`
    local WORKLOAD=0:${RANGEMAX}:${SKEWNESS}:${WRITEPERC}:${NUMQUERIES}

    local LOGFOLDER=${LOGFOLDER_NORMAL}
    local DATADEVICE=${DATADEVICE_NORMAL}

    echo "Starting ${OUTPUT_FNAME}"

    local CMD="${BIN_DIR}/select_only_${EXPR} --nthreads=${MPL} --inqlevel=${INQVRFYLEVEL} --bufferpool=${BUFFERPOOL_IN_KB} --warmup=${WARMUP} --workload=${WORKLOAD} --use-swizzling --log-folder ${LOGFOLDER} --data-device ${DATADEVICE} --readlatency=${EXTRAREADLATENCY} --oprofile --numa=${NUMANODE}"
    
    echo ${CMD}
    gdb --args ${CMD}
    #perf record ${CMD} 
    #${CMD} > ${OUTPUT_FNAME}
    #sudo LD_PRELOAD=${ZERO_BIN_DIR}/experiments/techcon14-nvram/libfs/libfs.so ${CMD}
    #${CMD}
    #CMD_PID=$!
    #wait ${CMD_PID}
    #echo 'PID='${CMD_PID}
    #top -b -n 1 | head -5 | awk '{printf"top: %s\n",$0}' >> ${OUTPUT_FNAME}
    #mpstat -P ALL  | awk '{printf"mpstat: %s\n",$0}' >> ${OUTPUT_FNAME}
}

# we control the machine speed using BIOS settings
# bufferpool size equals dataset size
run_expr_nvram_bufferpool() {
    local DATASET_SIZE_IN_KB=$1
    local RANGEMULT=$2
    local MPL=$3
    local SKEWNESS=$4
    local MEMSPEED=$5
    local NUMANODE=$6
    local REP=$7

    local BUFFERPOOL_MULT=1
    local BUFFERPOOL_IN_KB=${DATASET_SIZE_IN_KB}
    local NRECORDS=`size_to_nrecords ${DATASET_SIZE_IN_KB}`
    local RANGEMAX=`echo "${NRECORDS}*${RANGEMULT}" | bc -l | xargs printf "%1.0f"`
    local WARMUP="range:0:$RANGEMAX"
    #run_expr 'mainmemory-numa' ${SKEWNESS} 0 ${MPL} ${BUFFERPOOL_IN_KB} ${NRECORDS} ${RANGEMULT} $WARMUP 0 ${MEMSPEED} ${NUMANODE} ${REP}
    run_expr 'mainmemory' ${SKEWNESS} 0 ${MPL} ${BUFFERPOOL_IN_KB} ${NRECORDS} ${RANGEMULT} $WARMUP 0 ${MEMSPEED} ${NUMANODE} ${REP}
}

# we control the machine speed using BIOS settings
# bufferpool size equals dataset size
run_expr_nvram_backing_store() {
    local DATASET_SIZE_IN_KB=$1
    local BUFFERPOOL_MULT=$2
    local MPL=$3
    local SKEWNESS=$4
    local EXTRAREADLATENCY=$5
    local REP=$6
    
    local NUMANODE=0
    local BUFFERPOOL_IN_KB_LOG2=`echo "l(${DATASET_SIZE_IN_KB} * ${BUFFERPOOL_MULT})/l(2)" | bc -l`
    local BUFFERPOOL_IN_KB_LOG2_ROUNDUP=`echo "scale=0; ${BUFFERPOOL_IN_KB_LOG2}/1" | bc -l`
    local BUFFERPOOL_IN_KB=`echo "2^${BUFFERPOOL_IN_KB_LOG2_ROUNDUP}" | bc -l`
    local NRECORDS=`size_to_nrecords ${DATASET_SIZE_IN_KB}`
    local WARMUP="range:0:$NRECORDS"
    run_expr "noswizzling" ${SKEWNESS} 0 ${MPL} ${BUFFERPOOL_IN_KB} ${NRECORDS} 1 ${WARMUP} ${EXTRAREADLATENCY} ${MEMSPEED} ${NUMANODE} ${REP}
}

run_expr_nvram_bufferpool_multi() {
    local dataset_size_in_kb=$1
    #for rep in 1 2 3
    for rep in 1
    do
      for numanode in 0 1 2 4
      do
        for mpl in 1 2 4 8
        do
          for rangemult in 1
          do 
            for skew in 1
            do 
              run_expr_nvram_bufferpool ${dataset_size_in_kb} ${rangemult} ${mpl} ${skew} ${MEMSPEED} ${numanode} ${rep}
            done
          done
        done
      done
    done
}

run_expr_nvram_backing_store_multi() {
    local dataset_size_in_kb=$1

    for rep in 1 2 3
    #for rep in 1
    do
      for mpl in 1 2 4
      #for mpl in 1
      do
        #for extrareadlatency in 0 300 1000 2600 4700  # extra latencies used with the enthousiast machine (techcon 2014 submission)
        for extrareadlatency in 0 300 1000 1300 # extra latencies used with the dl-980 machine
        do 
          #for skew in 0 1 1.5 2 2.5
          for skew in 1
          do 
#            run_expr_nvram_backing_store ${dataset_size_in_kb} 0.085 ${mpl} ${skew} ${extrareadlatency} ${rep}
            run_expr_nvram_backing_store ${dataset_size_in_kb} 0.5 ${mpl} ${skew} ${extrareadlatency} ${rep}
            run_expr_nvram_backing_store ${dataset_size_in_kb} 0.25 ${mpl} ${skew} ${extrareadlatency} ${rep}
            run_expr_nvram_backing_store ${dataset_size_in_kb} 0.125 ${mpl} ${skew} ${extrareadlatency} ${rep}
            run_expr_nvram_backing_store ${dataset_size_in_kb} 0.0625 ${mpl} ${skew} ${extrareadlatency} ${rep}
          done
        done
      done
    done
}


check_or_makedirs

MEMSPEED=DL980

export LD_PRELOAD=${ZERO_BIN_DIR}/experiments/techcon14-nvram/libfs/libfs.so

dataset_size_in_kb=`echo "12*1024*1024" | bc -l`
#dataset_size_in_kb=`echo "4*1024*1024" | bc -l`
#dataset_size_in_kb=`echo "1*1024*1024" | bc -l`

#initdb_nrecords `size_to_nrecords ${dataset_size_in_kb}`

#run_expr_nvram_backing_store ${dataset_size_in_kb} 0.5 4 1 0 1

##run_expr_nvram_backing_store ${dataset_size_in_kb} 0.25 1 0 0 1
##run_expr_nvram_backing_store ${dataset_size_in_kb} 0.25 1 1 10000 1
##run_expr_nvram_bufferpool ${dataset_size_in_kb} 1 1 1 2933 1
#run_expr_nvram_bufferpool_multi ${dataset_size_in_kb}
run_expr_nvram_backing_store_multi ${dataset_size_in_kb}
