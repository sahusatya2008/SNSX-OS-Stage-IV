#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

VM_NAME="${VM_NAME:-SNS}"
ISO_PATH="${ISO_PATH:-${ROOT_DIR}/SNSX-VBox-ARM64.iso}"
VM_FRONTEND="${VM_FRONTEND:-gui}"
VM_REFRESH_MODE="${VM_REFRESH_MODE:-fresh}"
HTTPS_LOG="${HTTPS_LOG:-${ROOT_DIR}/.snsx-https-bridge.log}"
DEV_LOG="${DEV_LOG:-${ROOT_DIR}/.snsx-dev-bridge.log}"

wait_for_health() {
    local url="$1"
    local expected="$2"

    for _ in $(seq 1 20); do
        if curl -fsS "${url}" 2>/dev/null | grep -q "${expected}"; then
            return 0
        fi
        sleep 1
    done
    return 1
}

start_bridge_background() {
    local mode="$1"
    local log_path="$2"
    local health_url="$3"
    local expected="$4"
    local script_path

    case "${mode}" in
        https) script_path="${ROOT_DIR}/tools/run_https_bridge.sh" ;;
        dev) script_path="${ROOT_DIR}/tools/run_dev_bridge.sh" ;;
        *)
            echo "Unknown bridge mode: ${mode}" >&2
            exit 1
            ;;
    esac

    if wait_for_health "${health_url}" "${expected}"; then
        echo "SNSX ${mode} bridge is already ready."
        return 0
    fi

    nohup "${script_path}" >"${log_path}" 2>&1 &
    disown || true

    if ! wait_for_health "${health_url}" "${expected}"; then
        echo "SNSX ${mode} bridge did not become ready. Check ${log_path}." >&2
        exit 1
    fi
}

if [[ ! -f "${ISO_PATH}" ]]; then
    echo "ISO not found: ${ISO_PATH}" >&2
    echo "Build it first with: make vbox-arm64" >&2
    exit 1
fi

if [[ "${VM_REFRESH_MODE}" == "fresh" ]]; then
    REPLACE=1 RECREATE_DISK=1 "${ROOT_DIR}/tools/create_virtualbox_arm64_vm.sh" "${VM_NAME}" "${ISO_PATH}"
else
    "${ROOT_DIR}/tools/create_virtualbox_arm64_vm.sh" "${VM_NAME}" "${ISO_PATH}"
fi

start_bridge_background "https" "${HTTPS_LOG}" "http://127.0.0.1:8787/health" "SNSX HTTPS bridge is ready."
start_bridge_background "dev" "${DEV_LOG}" "http://127.0.0.1:8790/health" "SNSX Dev bridge is ready."

if VBoxManage showvminfo "${VM_NAME}" --machinereadable 2>/dev/null | grep -q '^VMState="running"$'; then
    echo "VM '${VM_NAME}' is already running."
    exit 0
fi

VBoxManage startvm "${VM_NAME}" --type "${VM_FRONTEND}"

echo
echo "SNSX is starting online."
echo "VM name: ${VM_NAME}"
echo "Frontend: ${VM_FRONTEND}"
echo "HTTPS bridge log: ${HTTPS_LOG}"
echo "Dev bridge log: ${DEV_LOG}"
