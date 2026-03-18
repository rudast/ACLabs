#!/bin/bash
set -e

# Конфигурация
PORT=5001
TEST_IMG="base_image.ppm"
TASKS=10
CONSUMERS_LIST=(1 2)

# Директории
IMG_DIR="./images"
OUT_DIR="./output"

# 1. Сборка
make -j$(nproc)

# 2. Генерация валидного бинарного P6 файла
if [ ! -f "$TEST_IMG" ]; then
    echo "Генерация $TEST_IMG..."
    printf "P6\n10 10\n255\n" > "$TEST_IMG"
    # 10x10 * 3 байта = 300 байт
    for i in {1..300}; do printf "\x7f"; done >> "$TEST_IMG"
fi

for C in "${CONSUMERS_LIST[@]}"; do
    echo "-----------------------------------"
    echo "Тест с $C consumer(s) и $TASKS задачами"
    
    # Очистка
    rm -rf "$IMG_DIR" "$OUT_DIR"
    mkdir -p "$IMG_DIR" "$OUT_DIR"
    rm -f *.log

    # Подготовка изображений
    for ((i=1; i<=TASKS; i++)); do
        cp "$TEST_IMG" "$IMG_DIR/img_${i}.ppm"
    done

    # 3. Запуск брокера
    ./build/broker --host 127.0.0.1 --port $PORT > broker.log 2>&1 &
    BR_PID=$!
    sleep 1

    # 4. Запуск консьюмеров
    CON_PIDS=()
    for ((i=1; i<=C; i++)); do
        ./build/consumer --host 127.0.0.1 --port $PORT > "consumer_${i}.log" 2>&1 &
        CON_PIDS+=($!)
    done
    sleep 1

    # 5. Запуск продюсера и замер времени
    START_TIME=$(date +%s.%N)
    ./build/producer --host 127.0.0.1 --port $PORT > producer.log 2>&1
    
    # Ожидание появления всех файлов
    COUNTER=0
    while [ $(ls -1 "$OUT_DIR" 2>/dev/null | wc -l) -lt $TASKS ] && [ $COUNTER -lt 50 ]; do
        sleep 0.2
        ((COUNTER++))
    done
    END_TIME=$(date +%s.%N)

    # 6. Расчет метрик
    DURATION=$(echo "$END_TIME - $START_TIME" | bc)
    TPS=$(echo "scale=2; $TASKS / $DURATION" | bc)

    echo "Время выполнения: ${DURATION} сек"
    echo "TPS (задач/сек): ${TPS}"

    # 7. Завершение процессов
    kill $BR_PID "${CON_PIDS[@]}" 2>/dev/null || true
    sleep 1
done
