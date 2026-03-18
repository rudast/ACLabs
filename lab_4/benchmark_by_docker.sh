#!/bin/bash
set -e

PORT=5001
TEST_IMG="base_image.ppm"
TASKS=250
CONSUMERS_LIST=(1 2 4 8 16)

DOCKER_CMD="docker compose"

if [ ! -f "$TEST_IMG" ]; then
    echo "Generating $TEST_IMG..."
    printf "P6\n500 500\n255\n" > "$TEST_IMG"
    for i in {1..750000}; do printf "\x7f"; done >> "$TEST_IMG"
fi

echo "Building..."
$DOCKER_CMD build -q

for C in "${CONSUMERS_LIST[@]}"; do
    echo "-----------------------------------"
    echo "Test consumer = $C, tasks = $TASKS "
    
    rm -rf images output
    mkdir -p images output
    
    for ((i=1; i<=TASKS; i++)); do
        cp "$TEST_IMG" "images/img_${i}.ppm"
    done

    $DOCKER_CMD up -d --scale consumer=$C broker consumer
    
    sleep 3 

    START_TIME=$(date +%s.%N)
    $DOCKER_CMD run --rm producer --host broker --port $PORT > /dev/null
    
    while [ $(ls -1q output 2>/dev/null | wc -l) -lt $TASKS ]; do
        sleep 0.5
    done
    END_TIME=$(date +%s.%N)

    DURATION=$(echo "$END_TIME - $START_TIME" | bc)
    TPS=$(echo "scale=2; $TASKS / $DURATION" | bc)
    
    echo "Runtime: ${DURATION} s."
    echo "TPS: ${TPS}"

    $DOCKER_CMD down -v > /dev/null 2>&1
done