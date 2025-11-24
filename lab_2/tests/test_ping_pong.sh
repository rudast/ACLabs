#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

SERVER_BIN="$ROOT_DIR/build/server"
CLIENT_BIN="$ROOT_DIR/build/client"

# Запускаем сервер в фоне
"$SERVER_BIN" &
SERVER_PID=$!

# Даём серверу время подняться
sleep 0.3

# Гоним сценарий: ping, потом q
CLIENT_OUTPUT=$(printf "ping\nq\n" | "$CLIENT_BIN")

echo "=== Client output ==="
echo "$CLIENT_OUTPUT"
echo "====================="

# Проверяем, что клиент получил pong
if echo "$CLIENT_OUTPUT" | grep -q "Client received message by Server: pong"; then
    echo "[OK] ping → pong"
    TEST_RESULT=0
else
    echo "[FAIL] pong от сервера не обнаружен"
    TEST_RESULT=1
fi

# Аккуратно гасим сервер
kill "$SERVER_PID" 2>/dev/null || true
wait "$SERVER_PID" 2>/dev/null || true

exit $TEST_RESULT
