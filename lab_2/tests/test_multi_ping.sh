#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

SERVER_BIN="$ROOT_DIR/build/server"
CLIENT_BIN="$ROOT_DIR/build/client"

"$SERVER_BIN" &
SERVER_PID=$!

sleep 0.3

CLIENT_OUTPUT=$(printf "ping\nping\nping\nq\n" | "$CLIENT_BIN")

echo "=== Client output ==="
echo "$CLIENT_OUTPUT"
echo "====================="

PONG_COUNT=$(echo "$CLIENT_OUTPUT" | grep -c "Client received message by Server: pong" || true)

if [ "$PONG_COUNT" -eq 3 ]; then
    echo "[OK] 3 ping → 3 pong"
    TEST_RESULT=0
else
    echo "[FAIL] ожидалось 3 pong, получили: $PONG_COUNT"
    TEST_RESULT=1
fi

kill "$SERVER_PID" 2>/dev/null || true
wait "$SERVER_PID" 2>/dev/null || true

exit $TEST_RESULT
