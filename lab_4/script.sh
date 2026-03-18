#!/bin/bash
set -e

# Конфигурация теста
PORT=5001  # Должен совпадать с docker-compose.yaml
TEST_IMG="base_image.ppm"
TASKS=10
CONSUMERS_LIST=(1 2)

DOCKER_CMD="sudo docker compose"

# 1. Генерация валидного P6 изображения
if [ ! -f "$TEST_IMG" ]; then
    printf "P6\n10 10\n255\n" > "$TEST_IMG"
    head -c 300 /dev/zero >> "$TEST_IMG"
fi

echo "Сборка образов..."
$DOCKER_CMD build -q

for C in "${CONSUMERS_LIST[@]}"; do
    echo "-----------------------------------"
    echo "Тест с $C consumer(s) и $TASKS задачами"
    
    # 2. Очистка (с sudo для WSL/Docker)
    sudo rm -rf images output
    mkdir -p images output
    
    for ((i=1; i<=TASKS; i++)); do
        cp "$TEST_IMG" "images/img_${i}.ppm"
    done

    # 3. Запуск инфраструктуры
    $DOCKER_CMD up -d --scale consumer=$C broker consumer
    
    # Ожидание готовности брокера (важно для WSL)
    sleep 3 

    # 4. Запуск продюсера (передаем порт 5001)
    START_TIME=$(date +%s.%N)
    $DOCKER_CMD run --rm producer --host broker --port $PORT > /dev/null
    
    # 5. Ожидание завершения
    while [ $(ls -1q output 2>/dev/null | wc -l) -lt $TASKS ]; do
        sleep 0.5
    done
    END_TIME=$(date +%s.%N)

    DURATION=$(echo "$END_TIME - $START_TIME" | bc)
    TPS=$(echo "scale=2; $TASKS / $DURATION" | bc)

    echo "Время выполнения: ${DURATION} сек"
    echo "TPS: ${TPS}"

    $DOCKER_CMD down -v > /dev/null 2>&1
done