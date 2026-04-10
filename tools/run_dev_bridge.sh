#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

HOST="${SNSX_DEV_BRIDGE_HOST:-0.0.0.0}"
HEALTH_HOST="${SNSX_DEV_BRIDGE_HEALTH_HOST:-127.0.0.1}"
PORT="${SNSX_DEV_BRIDGE_PORT:-8790}"

bridge_health() {
    python3 - "$HEALTH_HOST" "$PORT" <<'PY'
import sys
import urllib.request

host = sys.argv[1]
port = sys.argv[2]
try:
    with urllib.request.urlopen(f"http://{host}:{port}/health", timeout=2) as response:
        body = response.read(128).decode("utf-8", errors="replace")
        if response.status == 200 and "SNSX Dev bridge is ready." in body:
            raise SystemExit(0)
except Exception:
    pass
raise SystemExit(1)
PY
}

if bridge_health; then
    echo "SNSX Dev bridge is already running on http://${HOST}:${PORT}"
    exit 0
fi

pkill -f "tools/snsx_dev_bridge.py" >/dev/null 2>&1 || true
sleep 1

if lsof -nP -iTCP:"${PORT}" -sTCP:LISTEN >/dev/null 2>&1; then
    echo "Port ${PORT} is still in use by another process. Stop it first, then rerun make dev-bridge." >&2
    lsof -nP -iTCP:"${PORT}" -sTCP:LISTEN >&2 || true
    exit 1
fi

exec python3 "${ROOT_DIR}/tools/snsx_dev_bridge.py" --host "${HOST}" --port "${PORT}"
