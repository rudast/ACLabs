#!/bin/bash

logDisk=$1
backupDisk=$2

if [[ ! -d "$logDisk" ]]; then
    echo "[ERROR]: Set true path to the disk with /log as first parametr."
    exit 1
fi

if [[ ! -d "$backupDisk" ]]; then
    echo "[ERROR]: Set true path to the disk with /backup as second parametr."
fi

testsCount=4
countGeneratingFiles=('100' '300' '450' '600')
percentageForTests=('10' '40' '70' '90')
countForCompression=('70' '200' '420' '550')
touch "log.txt"
echo -n > "log.txt"

rm -rf "${logDisk:?}/"*
rm -rf "${backupDisk:?}/"*

for ((i=0; i<testsCount; i++)); do
    echo "[TEST] Starting test #$((i+1)) ..."
    testStart=$(date +%s)

    echo "===================================================" >> "log.txt"
    echo "                 [ TEST CASE #$((i+1)) ] " >> "log.txt"
    echo "===================================================" >> "log.txt"
    echo "Count of generated files      - ${countGeneratingFiles[i]};" >> "log.txt"
    echo "Percantage for compression    - ${percentageForTests[i]}%;" >> "log.txt"
    echo "Count to compression files    - ${countForCompression[i]};" >> "log.txt"
    echo "---------------------------------------------------" >> "log.txt"
    echo "" >> "log.txt"

    echo "Data are generated..." >> "log.txt"

    for ((j=0; j<countGeneratingFiles[i]; j++)); do
        touch "${logDisk}/testFile_${j}.txt"
        yes "test" | head -n 200000 > "${logDisk}/testFile_${j}.txt"
    done

    echo "Checker is working..." >> "log.txt"

    checkerStart=$(date +%s)
    bash checker.sh $logDisk $backupDisk ${percentageForTests[i]} ${countForCompression[i]} >> "log.txt"
    checkerFinish=$(date +%s)

    echo "Data are deleted..." >> "log.txt"

    rm -rf "${logDisk:?}/"*
    rm -rf "${backupDisk:?}/"*

    testFinish=$(date +%s)
    testTime=$((testFinish - testStart))
    checkerTime=$((checkerFinish - checkerStart))
    
    echo "[TEST] Test #$((i+1)) finished in ${testTime}s"

    echo "" >> "log.txt"
    echo "---------------------------------------------------" >> "log.txt"
    echo "Checker runtime   - ${checkerTime} seconds" >> "log.txt"
    echo "Test runtime      - ${testTime} seconds" >> "log.txt"
    echo "===================================================" >> "log.txt"
    echo "               [ TEST CASE #$((i+1)) - OK ] " >> "log.txt"
    echo "===================================================" >> "log.txt"
    echo "" >> "log.txt"
    echo "" >> "log.txt"

done


scriptDir="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
echo "[TEST] Completed $testsCount/$testsCount tests."
echo "[TEST] A detailed test log can be viewed at ${scriptDir}/log.txt."