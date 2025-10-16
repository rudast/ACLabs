#!/bin/bash

logDisk=$1
backupDisk=$2
logFile="${3:-tests.log}"

if [[ ! -d "$logDisk" ]]; then
    echo "[ERROR]: Set true path to the disk with /log as first parameter."
    exit 1
fi

if [[ ! -d "$backupDisk" ]]; then
    echo "[ERROR]: Set true path to the disk with /backup as second parameter."
    exit 1
fi

> "$logFile"

testsCount=8
countGeneratingFiles=(158 158 220 1 1 220 600 20)
percentageForTests=(50 69 0 1 2 90 99 70)
countForCompression=(2 0 0 0 300 1 100 5)

rm -rf "${logDisk:?}/"*
rm -rf "${backupDisk:?}/"*

for ((i=0; i<testsCount; i++)); do
    echo "[TEST] Starting test #$((i+1)):"
    testStart=$(date +%s)

    echo "===================================================" >> "$logFile"
    echo "                 [ TEST CASE #$((i+1)) ] " >> "$logFile"
    echo "===================================================" >> "$logFile"
    echo "Count of generated files      - ${countGeneratingFiles[i]};" >> "$logFile"
    echo "Percentage for compression    - ${percentageForTests[i]}%;" >> "$logFile"
    echo "Count to preserve newest      - ${countForCompression[i]};" >> "$logFile"
    echo "---------------------------------------------------" >> "$logFile"
    echo "" >> "$logFile"

    echo "Generating test files..." >> "$logFile"
    for ((j=0; j<countGeneratingFiles[i]; j++)); do
        filePath="${logDisk}/testFile_${j}.log"
        touch "$filePath"
        if [[ $i -eq 6 ]]; then
            echo "small content $j" > "$filePath"
        elif [[ $i -eq 7 ]]; then
            yes "big file content $j" | head -n 2000000 > "$filePath"
        else
            yes "test $j" | head -n 500000 > "$filePath"
        fi
    done

    echo "Running checker..." >> "$logFile"
    checkerStart=$(date +%s)
    bash checker.sh "$logDisk" "$backupDisk" "${percentageForTests[i]}" "${countForCompression[i]}" >> "$logFile" 2>&1
    checkerFinish=$(date +%s)

    echo "Cleaning up test directories..." >> "$logFile"
    rm -rf "${logDisk:?}/"*
    rm -rf "${backupDisk:?}/"*

    testFinish=$(date +%s)
    testTime=$((testFinish - testStart))
    checkerTime=$((checkerFinish - checkerStart))
    
    echo "[TEST] Test #$((i+1)) finished in ${testTime}s"

    echo "" >> "$logFile"
    echo "---------------------------------------------------" >> "$logFile"
    echo "Checker runtime   - ${checkerTime} seconds" >> "$logFile"
    echo "Test runtime      - ${testTime} seconds" >> "$logFile"
    echo "===================================================" >> "$logFile"
    echo "               [ TEST CASE #$((i+1)) - OK ] " >> "$logFile"
    echo "===================================================" >> "$logFile"
    echo "" >> "$logFile"
    echo "" >> "$logFile"

done

scriptDir="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
echo "[TEST] Completed $testsCount/$testsCount tests."
echo "[TEST] Detailed test log at ${scriptDir}/${logFile}."
