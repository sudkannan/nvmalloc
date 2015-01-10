#!/bin/zsh

DELIMITER=','

. ../../scripts/parse_common.sh

effective_readlatency_dl980 () {
  local memspeed=$1
  local numanode=$2
  local extrareadlatency=$3

  latencymap=("0_0=120" "1_0=162" "2_0=261" "4_0=295" "0_300=162" "0_1000=261" "0_1300=295")
  for pair in $latencymap; do
    if [[ "$pair" =~ "${numanode}_${extrareadlatency}" ]]; then
      echo `echo ${pair} | awk -F= '{print $2}'`
      return
    fi
  done
}

effective_readlatency () {
  local memspeed=$1
  local numanode=$2
  local extrareadlatency=$3

  echo $(effective_readlatency_dl980 $memspeed $numanode $extrareadlatency)
}

processfile_numqueries_techcon () {
  FNAME=${1}
  BNAME=`basename ${FNAME}`
  EXPR=`echo ${BNAME} | awk -F_ '{print $3}'` 
  SKEWNESS=`echo ${BNAME} | awk -F_ '{print $6}' | sed -e "s/SKEWNESS//"`
  WRITEPERC=`echo ${BNAME} | awk -F_ '{print $7}' | sed -e "s/WRITEPERC//"`
  INSERTPERC=`echo ${BNAME} | awk -F_ '{print $8}' | sed -e "s/INSERTPERC//"`
  DELETEPERC=`echo ${BNAME} | awk -F_ '{print $9}' | sed -e "s/DELETEPERC//"`
  MPL=`echo ${BNAME} | awk -F_ '{print $10}' | sed -e "s/MPL//"`
  RANGEMULT=`echo ${BNAME} | awk -F_ '{print $11}' | sed -e "s/RANGEMULT//"`
  MEMSPEED=`echo ${BNAME} | awk -F_ '{print $12}' | sed -e "s/MEMSPEED//"`
  NUMANODE=`echo ${BNAME} | awk -F_ '{print $13}' | sed -e "s/NUMANODE//"`
  EXTRAREADLATENCY=`echo ${BNAME} | awk -F_ '{print $14}' | sed -e "s/EXTRAREADLATENCY//"`
  BUFFERPOOL=`echo ${BNAME} | awk -F_ '{print $15}' | sed -e "s/BUFFERPOOL//"`
  NUMTRANS=`grep "total_transactions" ${FNAME} | awk -F= '{print $2}' | bc -l`

  REPCOUNT=0
  TIME=()
  DEADLOCK=()
  MISSRATIO=()
  EFFECTIVEREADLATENCY=$(effective_readlatency ${MEMSPEED} ${NUMANODE} ${EXTRAREADLATENCY})
  COMMONFNAME=`echo ${FNAME} | sed -e "s/REP1/REP/"`
  for FNAME in `ls ${COMMONFNAME}*`
  do
    REPCOUNT=$((REPCOUNT + 1)) 
    TIME[${REPCOUNT}]=`grep "elapsed time" ${FNAME} | awk -F= '{print $2}' | sed -e "s/ sec//"`
#    TIME[${REPCOUNT}]=`grep "workload time" ${FNAME} | awk -F= '{print $2}' | sed -e "s/ sec//"`
    MISSRATIO[${REPCOUNT}]=`grep "CACHE_MISS_RATIO" ${FNAME} | awk -F= '{print $2}' | sed -e "s/ sec//"`
  done
  
  AVGTIME=$(average TIME ${REPCOUNT})
  SDTIME=$(standarddeviation TIME ${REPCOUNT} ${AVGTIME})
  SETIME=$(standarderror ${REPCOUNT} ${SDTIME})
  if [ $#MISSRATIO -ge 1 ]
  then
    AVGMISSRATIO=$(average MISSRATIO ${REPCOUNT})
  else 
    AVGMISSRATIO=0
  fi

  if [ ! -z "${NUMTRANS}" ] && [ ! -z "${AVGTIME}" ]
  then 
       AVGTHROUGHPUT=`echo "${NUMTRANS} / (${AVGTIME})" | bc -l` 
       SDTHROUGHPUT=`echo "${AVGTHROUGHPUT} * (${SDTIME}) / ${AVGTIME}" | bc -l` 
       SETHROUGHPUT=$(standarderror ${REPCOUNT} ${SDTHROUGHPUT})
       TIMEPERTRAN=`echo "(${AVGTIME}  / ${NUMTRANS}) * 1000000" | bc -l` 
  fi
  echo "${EXPR}${DELIMITER}${BUFFERPOOL}${DELIMITER}${MEMSPEED}${DELIMITER}${NUMANODE}${DELIMITER}${EXTRAREADLATENCY}${DELIMITER}${EFFECTIVEREADLATENCY}${DELIMITER}${MPL}${DELIMITER}${SKEWNESS}${DELIMITER}${INSERTPERC}${DELIMITER}${DELETEPERC}${DELIMITER}${RANGEMULT}${DELIMITER}${AVGTIME}${DELIMITER}${SETIME}${DELIMITER}${NUMTRANS}${DELIMITER}${AVGTHROUGHPUT}${DELIMITER}${SETHROUGHPUT}${DELIMITER}${TIMEPERTRAN}${DELIMITER}${AVGMISSRATIO}" >>  ${DATFILE}
}

parse_all_techcon() {
    FILE_PREFIX=$1
    FNAME_LIST=`ls ${TARGETDIR} | grep "^${FILE_PREFIX}_.*REP1$"`
    if [ -n "$FNAME_LIST" ]
    then
        DATFILE="${TARGETDIR}/CLV-ALL.dat"
        OLDFILE="${TARGETDIR}/CLV-ALL.dat"

        SUFFIX=0
        while [ -f ${OLDFILE} ]
        do
          SUFFIX=`expr ${SUFFIX} + 1`
          OLDFILE="${TARGETDIR}/CLV-ALL-${SUFFIX}.dat"
        done

        # if old CLV file exists, then move it to CLV-${SUFFIX}.dat 
        if [ -f ${DATFILE} ]
        then
          mv ${DATFILE} ${OLDFILE}
        fi
          
        echo "producing output file: ${DATFILE}"
        echo "#1:Expr${DELIMITER}2:Bufferpool${DELIMITER}3:MemFreq${DELIMITER}4:NumaNode${DELIMITER}5:ExtraReadLatency${DELIMITER}6:EffectiveReadLatency${DELIMITER}7:MPL${DELIMITER}8:Skewness${DELIMITER}9:Insertperc${DELIMITER}10:Deleteperc${DELIMITER}11:RangeMultiplier${DELIMITER}12:elapsedTimeInSeconds${DELIMITER}13:elapsedTimeInSeconds (error)${DELIMITER}14:numberOfTransactions${DELIMITER}15:numberOfTransactionsPerSecond${DELIMITER}16:numberOfTransactionsPerSecond (error)${DELIMITER}17:avgTimePerTransaction(microseconds)${DELIMITER}18:missRatio" > ${DATFILE}

        for fname in `ls ${TARGETDIR} | grep "^${FILE_PREFIX}_.*REP1$"`
        do 
            processfile_numqueries_techcon ${TARGETDIR}/${fname} 
        done
    fi
}
# MAIN BODY

if [ $# -lt 1 ]
then
  echo ${USAGE}
  echo ${SEMANTICS}
  echo "Defaulting to ./"
  TARGETDIR="./"
else
  TARGETDIR=${1}
fi


if [ ! -d "${TARGETDIR}/" ]
then
  echo "No such directory ${TARGETDIR} ."
  echo ${USAGE}
  echo ${SEMANTICS}
  exit
fi

parse_all_techcon "select_only"
