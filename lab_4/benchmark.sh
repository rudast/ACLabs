#!/bin/bash
set -e

PORT=5001
TEST_IMG="base_image.ppm"
TASKS=250
CONSUMERS_LIST=(1 2 4 8 16)

IMG_DIR="./images"
OUT_DIR="./output"

mkdir -p logs
make -j$(nproc)

if [ ! -f "$TEST_IMG" ]; then
    echo "Generating $TEST_IMG..."
    printf "P6\n500 500\n255\n" > "$TEST_IMG"
    for i in {1..750000}; do printf "\x7f"; done >> "$TEST_IMG"
fi

for C in "${CONSUMERS_LIST[@]}"; do
    echo "-----------------------------------"
    echo "Test consumer = $C, tasks = $TASKS "
    
    rm -rf "$IMG_DIR" "$OUT_DIR"
    mkdir -p "$IMG_DIR" "$OUT_DIR"

    for ((i=1; i<=TASKS; i++)); do
        cp "$TEST_IMG" "$IMG_DIR/img_${i}.ppm"
    done

    ./build/broker --host 127.0.0.1 --port $PORT > logs/broker.log 2>&1 &
    BR_PID=$!
    sleep 1

    CON_PIDS=()
    for ((i=1; i<=C; i++)); do
        ./build/consumer --host 127.0.0.1 --port $PORT > logs/"consumer_${i}.log" 2>&1 &
        CON_PIDS+=($!)
    done
    sleep 1

    START_TIME=$(date +%s.%N)
    ./build/producer --host 127.0.0.1 --port $PORT > logs/producer.log 2>&1
    
    COUNTER=0
    while [ $(ls -1 "$OUT_DIR" 2>/dev/null | wc -l) -lt $TASKS ] && [ $COUNTER -lt 50 ]; do
        sleep 0.2
        ((COUNTER++))
    done
    END_TIME=$(date +%s.%N)

    DURATION=$(echo "$END_TIME - $START_TIME" | bc)
    TPS=$(echo "scale=2; $TASKS / $DURATION" | bc)

    echo "Runtime: ${DURATION} s."
    echo "TPS (task per second): ${TPS}"

    kill $BR_PID "${CON_PIDS[@]}" 2>/dev/null || true
    sleep 1;
done