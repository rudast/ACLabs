#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

SERVER_BIN="$ROOT_DIR/build/server"
CLIENT_BIN="$ROOT_DIR/build/client"

"$SERVER_BIN" &
SERVER_PID=$!

sleep 0.3

CLIENT_OUTPUT=$(printf "ping\nq\n" | "$CLIENT_BIN")

echo "=== Client output ==="
echo "$CLIENT_OUTPUT"
echo "====================="

if echo "$CLIENT_OUTPUT" | grep -q "Client received message by Server: pong"; then
    echo "[OK] ping → pong"
    TEST_RESULT=0
else
    echo "[FAIL] pong от сервера не обнаружен"
    TEST_RESULT=1
fi

kill "$SERVER_PID" 2>/dev/null || true
wait "$SERVER_PID" 2>/dev/null || true

exit $TEST_RESULT
