#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

VM_NAME="${VM_NAME:-${1:-SNSX-ARM64}}"
ISO_PATH="${ISO_PATH:-${2:-${ROOT_DIR}/SNSX-VBox-ARM64.iso}}"
MEMORY_MB="${MEMORY_MB:-1024}"
REPLACE="${REPLACE:-0}"
VM_BASEFOLDER="${VM_BASEFOLDER:-${ROOT_DIR}/.snsx-vms}"
PERSIST_SIZE_MB="${PERSIST_SIZE_MB:-64}"
RECREATE_DISK="${RECREATE_DISK:-0}"
PERSIST_BASENAME="${PERSIST_BASENAME:-${VM_NAME}-PERSIST}"
PERSIST_RAW_PATH="${PERSIST_RAW_PATH:-${ROOT_DIR}/${PERSIST_BASENAME}.raw}"
PERSIST_VDI_PATH="${PERSIST_VDI_PATH:-${ROOT_DIR}/${PERSIST_BASENAME}.vdi}"

if ! command -v VBoxManage >/dev/null 2>&1; then
    echo "VBoxManage was not found. Install Oracle VirtualBox first." >&2
    exit 1
fi

if ! command -v qemu-img >/dev/null 2>&1; then
    echo "qemu-img was not found. Install qemu first." >&2
    exit 1
fi

if ! command -v mformat >/dev/null 2>&1; then
    echo "mformat was not found. Install mtools first." >&2
    exit 1
fi

if [[ ! -f "${ISO_PATH}" ]]; then
    echo "ISO not found: ${ISO_PATH}" >&2
    echo "Build it first with: make vbox-arm64" >&2
    exit 1
fi

vm_info_value() {
    local key="$1"

    VBoxManage showvminfo "${VM_NAME}" --machinereadable 2>/dev/null |
        awk -F= -v key="${key}" '$1 == key { gsub(/"/, "", $2); print $2; exit }'
}

kill_stale_vm_processes() {
    local uuid
    local cfg_file
    local pids=()

    uuid="$(vm_info_value UUID || true)"
    cfg_file="$(vm_info_value CfgFile || true)"

    while IFS= read -r line; do
        local pid
        local cmd

        [[ -z "${line}" ]] && continue
        pid="${line%% *}"
        cmd="${line#* }"
        if [[ "${cmd}" == *"${VM_NAME}"* ||
              (-n "${uuid}" && "${cmd}" == *"${uuid}"*) ||
              (-n "${cfg_file}" && "${cmd}" == *"${cfg_file}"*) ]]; then
            pids+=("${pid}")
        fi
    done < <(pgrep -af "VirtualBoxVM|VBoxHeadless" 2>/dev/null || true)

    if (( ${#pids[@]} == 0 )); then
        return
    fi

    echo "Closing stale VirtualBox session for '${VM_NAME}'..." >&2
    for pid in "${pids[@]}"; do
        kill "${pid}" >/dev/null 2>&1 || true
    done
    sleep 2
    for pid in "${pids[@]}"; do
        kill -0 "${pid}" >/dev/null 2>&1 || continue
        kill -9 "${pid}" >/dev/null 2>&1 || true
    done
    sleep 1
}

default_machine_folder() {
    VBoxManage list systemproperties 2>/dev/null |
        awk -F': *' '/^Default machine folder:/ { print $2; exit }'
}

wait_for_vm_mutable() {
    local max_wait="${1:-30}"
    local waited=0
    local recovered=0

    while (( waited < max_wait )); do
        local state
        local session_name

        state="$(vm_info_value VMState)"
        session_name="$(vm_info_value SessionName)"
        if [[ "${state}" == "poweroff" && -z "${session_name}" ]]; then
            return 0
        fi
        sleep 1
        ((waited += 1))
        if (( waited >= max_wait / 2 )) && (( recovered == 0 )); then
            recovered=1
            kill_stale_vm_processes
        fi
    done

    kill_stale_vm_processes
    for _ in $(seq 1 10); do
        local state
        local session_name

        state="$(vm_info_value VMState)"
        session_name="$(vm_info_value SessionName)"
        if [[ "${state}" == "poweroff" && -z "${session_name}" ]]; then
            return 0
        fi
        sleep 1
    done

    echo "Timed out waiting for VirtualBox VM '${VM_NAME}' to become mutable." >&2
    VBoxManage showvminfo "${VM_NAME}" --machinereadable >&2 || true
    exit 1
}

ensure_vm_powered_off() {
    local state

    state="$(vm_info_value VMState)"
    case "${state}" in
        ""|poweroff)
            ;;
        paused)
            VBoxManage controlvm "${VM_NAME}" poweroff >/dev/null 2>&1 || true
            ;;
        saved)
            VBoxManage discardstate "${VM_NAME}" >/dev/null 2>&1 || true
            ;;
        *)
            VBoxManage controlvm "${VM_NAME}" poweroff >/dev/null 2>&1 || true
            ;;
    esac

    wait_for_vm_mutable 40
}

detach_medium() {
    local controller="$1"
    local port="$2"

    VBoxManage storageattach "${VM_NAME}" \
        --storagectl "${controller}" \
        --port "${port}" \
        --device 0 \
        --medium none >/dev/null 2>&1 || true
}

close_disk_medium() {
    local path="$1"

    VBoxManage closemedium disk "${path}" >/dev/null 2>&1 || true
}

purge_disk_registry_entries() {
    local path="$1"
    local prefix="${2:-}"
    local uuids_text
    local -a uuids=()

    uuids_text="$(
        VBoxManage list hdds 2>/dev/null | awk -v target="${path}" -v prefix="${prefix}" '
            /^UUID:/ {
                uuid = $2
                next
            }
            /^Location:/ {
                location = $0
                sub(/^Location:[[:space:]]*/, "", location)
                if (uuid != "" && (location == target || (prefix != "" && index(location, prefix "/") == 1))) {
                    print uuid
                }
            }
        '
    )"

    if [[ -z "${uuids_text}" ]]; then
        return
    fi

    while IFS= read -r uuid; do
        [[ -z "${uuid}" ]] && continue
        uuids=("${uuid}" "${uuids[@]-}")
    done <<< "${uuids_text}"

    for uuid in "${uuids[@]}"; do
        VBoxManage closemedium disk "${uuid}" >/dev/null 2>&1 || true
    done
}

existing_arch=""
vm_cfg_file=""
if VBoxManage showvminfo "${VM_NAME}" >/dev/null 2>&1; then
    vm_cfg_file="$(vm_info_value CfgFile || true)"
    if [[ -n "${vm_cfg_file}" && ! -f "${vm_cfg_file}" ]]; then
        echo "Repairing broken VirtualBox registration for '${VM_NAME}'..." >&2
        REPLACE=1
    fi
    if [[ "${REPLACE}" == "1" ]]; then
        vm_cfg_dir=""
        if [[ -n "${vm_cfg_file}" ]]; then
            vm_cfg_dir="$(dirname "${vm_cfg_file}")"
        fi
        ensure_vm_powered_off
        VBoxManage unregistervm "${VM_NAME}" --delete >/dev/null 2>&1 || VBoxManage unregistervm "${VM_NAME}" >/dev/null 2>&1 || true
        if [[ -n "${vm_cfg_dir}" ]]; then
            rm -rf "${vm_cfg_dir}"
        fi
        purge_disk_registry_entries "${PERSIST_VDI_PATH}" "${vm_cfg_dir}"
    fi
fi

if [[ "${REPLACE}" == "1" ]]; then
    default_vm_dir="$(default_machine_folder)"
    if [[ -n "${default_vm_dir}" ]]; then
        rm -rf "${default_vm_dir}/${VM_NAME}"
        purge_disk_registry_entries "${PERSIST_VDI_PATH}" "${default_vm_dir}/${VM_NAME}"
    fi
fi

existing_arch=""
if VBoxManage showvminfo "${VM_NAME}" >/dev/null 2>&1; then
    existing_arch="$(VBoxManage showvminfo "${VM_NAME}" --machinereadable | awk -F= '/^platformArchitecture=/{gsub(/"/, "", $2); print $2}')"
    if [[ "${existing_arch}" != "ARM" && "${existing_arch}" != "arm" && -n "${existing_arch}" ]]; then
        if [[ "${REPLACE}" == "1" ]]; then
            echo "Replacing existing non-ARM VM: ${VM_NAME}"
            VBoxManage unregistervm "${VM_NAME}" --delete
        else
            echo "Existing VM '${VM_NAME}' is ${existing_arch}, not ARM64." >&2
            echo "Apple Silicon VirtualBox cannot boot x86 guests." >&2
            echo "Use a new VM name, or rerun with REPLACE=1 to recreate it as ARM64." >&2
            exit 1
        fi
    fi
fi

mkdir -p "${VM_BASEFOLDER}"

if ! VBoxManage showvminfo "${VM_NAME}" >/dev/null 2>&1; then
    VBoxManage createvm --name "${VM_NAME}" --platform-architecture arm --ostype Other_arm64 \
        --basefolder "${VM_BASEFOLDER}" --register
fi

ensure_vm_powered_off

mkdir -p "$(dirname "${PERSIST_RAW_PATH}")"
mkdir -p "$(dirname "${PERSIST_VDI_PATH}")"

detach_medium "SATA" 0
close_disk_medium "${PERSIST_VDI_PATH}"
purge_disk_registry_entries "${PERSIST_VDI_PATH}" "${default_vm_dir:-}"

if [[ "${RECREATE_DISK}" == "1" ]]; then
    rm -f "${PERSIST_RAW_PATH}" "${PERSIST_VDI_PATH}"
    purge_disk_registry_entries "${PERSIST_VDI_PATH}" "${default_vm_dir:-}"
fi

if [[ ! -f "${PERSIST_VDI_PATH}" ]]; then
    rm -f "${PERSIST_RAW_PATH}"
    qemu-img create -f raw "${PERSIST_RAW_PATH}" "${PERSIST_SIZE_MB}M" >/dev/null
    mformat -i "${PERSIST_RAW_PATH}" -F ::
    VBoxManage convertfromraw "${PERSIST_RAW_PATH}" "${PERSIST_VDI_PATH}" --format=VDI >/dev/null
fi

VBoxManage modifyvm "${VM_NAME}" \
    --memory "${MEMORY_MB}" \
    --vram 16 \
    --firmware efi \
    --chipset armv8virtual \
    --graphicscontroller qemuramfb \
    --audio-enabled off \
    --nic1 nat \
    --nat-localhostreachable1 on \
    --natdnsproxy1 on \
    --natdnshostresolver1 on \
    --usb on \
    --mouse usbtablet \
    --keyboard usb \
    --usb-ohci off \
    --usb-ehci off \
    --usb-xhci on \
    --boot1 dvd \
    --boot2 disk \
    --boot3 none \
    --boot4 none

if VBoxManage showvminfo "${VM_NAME}" --machinereadable | grep -q '="IDE"$'; then
    VBoxManage storageattach "${VM_NAME}" --storagectl IDE --port 0 --device 0 --medium none >/dev/null 2>&1 || true
    VBoxManage storageattach "${VM_NAME}" --storagectl IDE --port 1 --device 0 --medium none >/dev/null 2>&1 || true
    VBoxManage storagectl "${VM_NAME}" --name IDE --remove >/dev/null 2>&1 || true
fi

if ! VBoxManage showvminfo "${VM_NAME}" --machinereadable | grep -q '="SATA"$'; then
    VBoxManage storagectl "${VM_NAME}" --name SATA --add sata --controller IntelAhci --portcount 4 --bootable on
fi

detach_medium "SATA" 0
detach_medium "SATA" 1
close_disk_medium "${PERSIST_VDI_PATH}"

VBoxManage storageattach "${VM_NAME}" \
    --storagectl SATA \
    --port 0 \
    --device 0 \
    --type hdd \
    --medium "${PERSIST_VDI_PATH}"

VBoxManage storageattach "${VM_NAME}" \
    --storagectl SATA \
    --port 1 \
    --device 0 \
    --type dvddrive \
    --medium "${ISO_PATH}"

echo
echo "SNSX VirtualBox VM is ready."
echo "VM name: ${VM_NAME}"
echo "ISO: ${ISO_PATH}"
echo "Persistent disk: ${PERSIST_VDI_PATH}"
echo
echo "Start it from the VirtualBox GUI, or run:"
echo "VBoxManage startvm \"${VM_NAME}\" --type gui"
