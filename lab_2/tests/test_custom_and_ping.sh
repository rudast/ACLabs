#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

SERVER_BIN="$ROOT_DIR/build/server"
CLIENT_BIN="$ROOT_DIR/build/client"

"$SERVER_BIN" &
SERVER_PID=$!

sleep 0.3

# Отправим "hello", потом "ping", потом "q"
CLIENT_OUTPUT=$(printf "hello\nping\nq\n" | "$CLIENT_BIN")

echo "=== Client output ==="
echo "$CLIENT_OUTPUT"
echo "====================="

# Проверяем, что хотя бы один pong есть
PONG_COUNT=$(echo "$CLIENT_OUTPUT" | grep -c "Client received message by Server: pong" || true)

if [ "$PONG_COUNT" -ge 1 ]; then
    echo "[OK] после кастомного сообщения ping → pong отработал"
    TEST_RESULT=0
else
    echo "[FAIL] pong не обнаружен"
    TEST_RESULT=1
fi

kill "$SERVER_PID" 2>/dev/null || true
wait "$SERVER_PID" 2>/dev/null || true

exit $TEST_RESULT
