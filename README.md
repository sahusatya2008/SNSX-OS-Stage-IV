# SNSX Operating System Version 4

`SNSX` now has two bootable editions:

1. `SNSX.iso`
   The original `x86` GRUB build for Intel/AMD virtual machines.
2. `SNSX-VBox-ARM64.iso`
   The new `ARM64 UEFI` build made specifically for Oracle VirtualBox on Apple Silicon Macs.

If you are on an Apple Silicon Mac and want to use VirtualBox, the file you should attach is:

```text
SNSX-VBox-ARM64.iso
```

That is the important fix for your `VBOX_E_PLATFORM_ARCH_NOT_SUPPORTED` problem. VirtualBox on Apple Silicon boots `Arm64` guests, so I added a separate `Arm64` SNSX build for that platform.

## What Exists Now

### Apple Silicon VirtualBox Edition

The ARM64 VirtualBox build includes:

- UEFI AArch64 boot
- Boot-time input diagnostics screen
- First-run setup app before the desktop
- Mouse-driven graphical desktop in VirtualBox when GOP is available
- Auto-advance from diagnostics/setup into the desktop even if no input arrives yet
- Standalone SNSX shell
- Writable RAM filesystem
- Explicit `save` / `install` persistence flow
- Expanded Settings system for theme, mouse speed, pointer trail, network mode, host/profile names, and autosave
- Expanded Network Center for Ethernet/LAN state, preferred transport mode, Wi-Fi profile storage, hotspot profile storage, and autoconnect control
- Native Intel `82540EM` VirtualBox NIC driver path when firmware SNP is missing
- Real wired-network stack over UEFI SNP when firmware exposes the NIC: DHCP, IPv4 lease handling, DNS resolution, and plain HTTP fetch support
- Dedicated Wi-Fi profile dashboard for saved SSIDs, remembered passwords, and connect/forget flows
- Dedicated Bluetooth dashboard for saved pair/connect state and future hardware-backed pairing
- SNSX Browser app for bundled pages, tabs, bookmarks, history, downloads, network-status-aware browsing, and plain HTTP page fetches
- SNSX Code Studio app for `/projects` workspaces with host-backed run/build flows for SNSX, Python, JavaScript, C, C++, HTML, and CSS
- SNSX App Store app for apps, games, driver packs, and install kits
- Bare-metal install kit and guide for bootable USB or internal-disk ARM64 UEFI installs
- Expanded Disk Manager with active-volume selection plus install/save/rescan workflow
- File commands: `ls`, `tree`, `cd`, `open`, `find`, `cat`, `stat`, `touch`, `new`, `write`, `append`, `mkdir`, `rm`, `cp`, `mv`
- File manager app: `files`
- Calculator app: `calc`
- Text editor app: `edit FILE`
- Built-in install docs: `guide`, `/docs/INSTALL.TXT`, `/docs/QUICKSTART.TXT`
- VM helper that repairs stale VirtualBox disk registrations before reattaching the persistence disk

Important note:

- SNSX now has a much more complete network-management system in the UI and shell, including Wi-Fi, hotspot, Bluetooth, and browser dashboards.
- On Apple Silicon VirtualBox today, the real transport exposed to SNSX is still the virtual wired Ethernet/LAN path provided by firmware.
- SNSX now contains a real DHCP/IPv4/DNS/plain-HTTP stack for that wired firmware path.
- On the current `VirtualBox 7.2.x` ARM64 setup tested here, SNSX can see a PCI NIC in the guest, but the firmware still reports `SNP 0 / PXE 0 / IP4 0 / UDP4 0 / TCP4 0 / DHCP4 0`.
- SNSX now also contains a native `82540EM` path for the VirtualBox PCI NIC, and the current ARM64 build was re-tested with DHCP lease acquisition, DNS resolution, plain HTTP fetches such as `http http://example.com/`, and bridged HTTPS fetches such as `http https://example.com/`.
- Use `netdiag` or open `snsxdiag://network` in SNSX Browser to inspect the exact firmware-handle and PCI-hardware state.
- Real nearby Wi-Fi scanning, Bluetooth radio discovery/pairing, native guest-side TLS, and a Firefox-class browser engine still require future SNSX device drivers and substantially more runtime infrastructure.

### Original x86 Edition

The older x86 build is still present and still works on x86 virtual machines. It includes:

- GRUB ISO boot
- Custom BIOS boot image
- C + NASM monolithic kernel
- GDT, IDT, paging, PIT, keyboard IRQ, ATA probe
- Writable RAM VFS
- Shell apps and text-mode UI

## Build Requirements

On your Apple Silicon Mac, install:

```bash
brew install i686-elf-binutils i686-elf-gcc i686-elf-grub xorriso nasm qemu llvm lld mtools
```

## Build Commands

### Build the Apple Silicon VirtualBox ISO

```bash
make vbox-arm64
```

This creates:

```text
SNSX-VBox-ARM64.iso
```

### Start the optional host bridges

For `https://` pages in SNSX Browser:

```bash
make https-bridge
```

The bridge now listens on the NAT-reachable host interface so the guest can reach it at `10.0.2.2:8787`.

For SNSX Code Studio run/build actions:

```bash
make dev-bridge
```

The dev bridge also listens on the NAT-reachable host interface so the guest can reach it at `10.0.2.2:8790`.

Status and restart helpers:

```bash
make https-bridge-status
make https-bridge-stop
make dev-bridge-status
make dev-bridge-stop
```

### Run the ARM64 build locally in QEMU

```bash
make run-arm64
```

This boots the ARM64 VirtualBox edition.

If you specifically want the older serial/headless boot path:

```bash
make run-arm64-headless
```

### Run the ARM64 build with a writable persistence disk in QEMU

```bash
make run-arm64-persist
```

### Build the ISO and auto-create the correct ARM64 VirtualBox VM

```bash
make virtualbox-vm VM_NAME=SNSX
```

This builds `SNSX-VBox-ARM64.iso`, creates an `Arm64` EFI VM, attaches the ISO, and provisions a writable persistence disk for the SNSX Installer app.

Each VM now gets its own persistent disk by default, based on the VM name, so stale installed copies do not get accidentally reused across different SNSX VMs.
The helper now uses a SATA controller for ARM64 VirtualBox VMs, because legacy IDE/PIIX4 storage can fail during VM construction on newer Apple Silicon VirtualBox releases.
The helper now configures `usbkbd` + `usbtablet` with `xHCI` for ARM64 VirtualBox, which is a better fit for the graphical UEFI desktop flow on Apple Silicon.
The helper now also detaches and closes stale VirtualBox media registrations before it reattaches the persistence disk, which fixes the `UUID ... does not match the value stored in the media registry` error.

### Build the original x86 ISO

```bash
make
```

This creates:

```text
SNSX.iso
```

### Build the original custom x86 boot image

```bash
make custom-image
```

This creates:

```text
SNSX.img
```

## Bare-Metal ARM64 Procedure

SNSX can now prepare a primary-boot ARM64 UEFI layout on a real USB stick or internal FAT EFI partition, but this is still a guided install flow rather than a full disk partitioner.

1. Build `SNSX-VBox-ARM64.iso` with `make vbox-arm64`
2. Put `BOOTAA64.EFI` on a FAT32 EFI partition at `/EFI/BOOT/BOOTAA64.EFI`, or write the ISO to removable media
3. Boot the target laptop or board in `ARM64 UEFI` mode
4. Inside SNSX, open `Installer` or `Disk Manager`
5. Select the writable FAT target volume
6. Run `install`
7. Run `save`
8. Put that disk first in firmware boot order if SNSX should become the primary OS

The in-OS bare-metal guide is also bundled at `/docs/BAREMETAL.TXT`.

## Exact VirtualBox Procedure For Your Apple Silicon Mac

These are the steps you should follow now.

### 1. Build the correct ISO

From the SNSX project directory:

```bash
make vbox-arm64
```

Make sure this file exists:

```text
SNSX-VBox-ARM64.iso
```

Or let SNSX create the VM for you:

```bash
make virtualbox-vm VM_NAME=SNSX
```

That path now also creates:

```text
<VM_NAME>-PERSIST.vdi
```

and attaches it to the VM as the writable SNSX disk.

If you already had a broken or stale ARM64 VM, repair it with:

```bash
make virtualbox-repair VM_NAME=SNS
```

That refresh path reattaches the ISO, repairs stale disk registration state, and keeps the VM on the latest ARM64 hardware profile.

### One-command online launch

To rebuild the VM, start the browser HTTPS bridge, start the Code Studio dev bridge, and boot the VM in one flow:

```bash
make virtualbox-online VM_NAME=SNS
```

For testing without opening the GUI:

```bash
VM_FRONTEND=headless make virtualbox-online VM_NAME=SNS
```

### Browser navigation shortcuts

Inside the SNSX Browser UI:

- `1`-`9` opens extracted links shown on the current page
- `L` prompts for a link number to open
- `[` and `]` switch tabs
- `N` opens a new tab
- `M` toggles a bookmark for the current page
- The right-side `PAGE LINKS` panel is mouse-clickable for extracted links

### UI-first online flow

Once SNSX boots to the graphical desktop, you do not need the shell for the normal online path:

1. Click `Network` from the top bar or desktop card
2. SNSX will auto-attempt wired online bring-up when autoconnect is enabled
3. If the guest still shows `CONNECTING`, click `CONNECT`
4. Click `Browser` from the Network Center or desktop
5. Enter an `http://` or `https://` address in the browser address bar
6. Use the clickable `PAGE LINKS` panel on the right to open extracted links without the CLI
7. Open `Studio` from the desktop to run or build project files through the UI

On Apple Silicon VirtualBox, SNSX now also has a VirtualBox NAT fallback profile. If DHCP does not answer on the emulated `82540EM`, SNSX adopts the standard VirtualBox guest route:

- Guest IP: `10.0.2.15`
- Gateway: `10.0.2.2`
- DNS: `10.0.2.3`

That lets SNSX use the host Mac's active Wi-Fi or Ethernet connection through VirtualBox NAT instead of staying offline.

### 2. Open Oracle VirtualBox

Launch VirtualBox.

### 3. Create a new virtual machine

1. Click `New`
2. Set the VM name to:

```text
SNS
```

or:

```text
SNSX
```

3. Make sure the machine is an `Arm64` guest
4. Leave EFI/UEFI boot enabled
5. Give it at least `512 MB` RAM

If VirtualBox offers an unattended install option, turn it off. SNSX is not a Linux installer ISO.

Important:

- Do not reuse your old `SN` or `SNS` x86 VMs on Apple Silicon.
- That exact x86-vs-ARM mismatch is what caused `VBOX_E_PLATFORM_ARCH_NOT_SUPPORTED`.
- Create a new ARM64 VM instead, or recreate an old one with the helper script.

### 4. Attach the ISO

Open:

```text
Settings -> Storage
```

Attach this file to the virtual optical drive:

```text
SNSX-VBox-ARM64.iso
```

For persistent installs, also attach a writable virtual hard disk. If you use `make virtualbox-vm`, this is handled automatically.

### 5. Start the VM

Start the machine.

If the firmware opens a boot manager instead of directly starting SNSX:

1. Choose the optical drive entry
2. Or choose the UEFI CD/DVD boot option
3. Then continue booting

### 5A. One-command VM creation with VBoxManage

You can skip the manual VM creation and let SNSX configure it:

```bash
make virtualbox-vm VM_NAME=SNSX
```

Or run the helper directly:

```bash
./tools/create_virtualbox_arm64_vm.sh SNSX
```

If you already have an old x86 VM with that same name and want to replace it:

```bash
REPLACE=1 ./tools/create_virtualbox_arm64_vm.sh SNSX
```

### 6. First commands to try inside SNSX

Once SNSX starts, try:

```text
The first boot opens input diagnostics, then the SNSX First-Run Setup screen.
Choose DESKTOP to reach the launcher, or INSTALL and SAVE if you want persistence first.
help
apps
ls /home
open /docs/QUICKSTART.TXT
append /home/JOURNAL.TXT first boot on VirtualBox
theme sunrise
pointer 3
netmode ethernet
hostname SNSX-LAB
save
```

### 7. Install SNSX onto the writable VM disk

After the desktop appears:

1. Press `Enter` for the shell
2. Run `install`
3. Run `save`
4. Optionally edit files such as `/home/NOTES.TXT`
5. Run `save` again if you changed anything else
6. Shut down the VM
7. Remove the ISO from the optical drive if you want to boot from disk only
8. Boot the VM again

On later boots with the persistent disk attached, SNSX restores:

- files you saved into the SNSX filesystem
- install state
- theme preference
- desktop/settings state

### Shell Commands

Inside the SNSX terminal, these VirtualBox-related commands are now available:

```text
welcome
settings
theme
theme ocean
theme sunrise
theme slate
theme emerald
```

### Recommended VirtualBox Flow

For the smoothest setup on Apple Silicon:

1. Run `make virtualbox-online VM_NAME=SNS`
2. Let the diagnostics screen auto-continue or press `Enter`
3. At `SNSX First-Run Setup`, choose `INSTALL` and then `SAVE`, or just wait for auto-desktop
4. Choose `DESKTOP` if it has not already auto-opened
5. Open `Terminal` inside SNSX and run `ip`, `dns example.com`, `http http://example.com/`, and `http https://example.com/`
6. Open `Studio` inside SNSX and try `run /projects/main.snsx` or `studio`
7. Power off the VM

If you prefer the manual path instead:

1. Run `make virtualbox-fresh VM_NAME=SNS`
2. In a second host terminal, run `make https-bridge`
3. In a third host terminal, run `make dev-bridge`
4. Start the VM with `VBoxManage startvm "SNS" --type gui`
9. Eject the ISO if you want disk-only boot
10. Start the VM again from the attached persistent disk

`make virtualbox-fresh` now powers off a running `SNS` VM, clears stale snapshot/media registrations, recreates the writable disk, rebuilds the VM cleanly before reattaching the latest ISO, and enables VirtualBox NAT localhost reachability for the SNSX HTTPS bridge path.

### If You See "Synchronous Exception"

If VirtualBox shows a black screen with `Synchronous Exception`, use this recovery flow:

1. Replace the attached ISO with the latest [SNSX-VBox-ARM64.iso](/Volumes/Blockchain%20Drive/HighLevelSoftwares/Incomplete%20Project/OS/SNSX-VBox-ARM64.iso)
2. Make sure the VM boots from the optical drive first
3. If you already installed SNSX to the writable disk, temporarily detach that disk or force DVD boot once
4. Boot the fixed ISO
5. Run `install`
6. Run `save`
7. Shut down the VM
8. Reattach the writable disk if you detached it, then boot again

The current build restores the framebuffer desktop path and adds a diagnostics + setup sequence before the desktop, so you no longer land in the older shell-first recovery flow by default.

### If You See "Display output is not active."

This usually means the VM is still booting through an old installed disk state or a firmware display path that never reached the fixed ISO cleanly.

Try this in order:

1. Run `make virtualbox-refresh VM_NAME=SNSX`
2. In VirtualBox, confirm the VM boots from `Optical` before `Hard Disk`
3. Attach the latest [SNSX-VBox-ARM64.iso](/Volumes/Blockchain%20Drive/HighLevelSoftwares/Incomplete%20Project/OS/SNSX-VBox-ARM64.iso)
4. If the problem persists, do a fresh VM disk reset:

```bash
make virtualbox-fresh VM_NAME=SNSX
```

That recreates the writable disk for that VM and removes the stale installed SNSX copy that may still be booting.

### If VirtualBox Shows Host-Side Keyboard Failure Banners

The popups that mention `putScancodes` are generated by VirtualBox on the host, not by SNSX itself.

If you still see those banners on macOS:

1. Quit VirtualBox completely
2. Open `System Settings -> Privacy & Security -> Input Monitoring`
3. Enable access for `VirtualBox` and `VirtualBoxVM` if macOS lists them
4. Also check `Privacy & Security -> Accessibility` and enable the same apps
5. Start the VM again

SNSX now auto-continues into the desktop even when early firmware keyboard events are unreliable, but the host VirtualBox app still needs normal macOS keyboard permission if it wants to inject host keyboard state cleanly.

### If You See A VirtualBox Medium UUID Mismatch

If VirtualBox shows an error like:

```text
UUID {...} of the medium 'SNS-PERSIST.vdi' does not match the value stored in the media registry
```

run:

```bash
make virtualbox-repair VM_NAME=SNS
```

If you want a complete reset of the VM and its persistence disk:

```bash
make virtualbox-fresh VM_NAME=SNS
```

### Older Guidance

The stable install flow is:

1. Press `Enter` at the desktop for the shell
2. Run `install`
3. Run `save`
4. Shut down the VM
5. Remove the ISO from the optical drive if you want to boot from disk only
6. Boot the VM again

At that point SNSX can boot from the installed disk, and saved files such as `/home/NOTES.TXT` or `/home/JOURNAL.TXT` persist across reboots.

## Apple Silicon VirtualBox File Summary

For your Mac:

```text
Use SNSX-VBox-ARM64.iso in VirtualBox
```

Do not use this file on Apple Silicon VirtualBox:

```text
SNSX.iso
```

That file is the older x86 build and is only for x86 virtual machines.

## ARM64 SNSX Commands

The Apple Silicon VirtualBox edition supports:

- Boot-time diagnostics
- First-run setup
- Graphical desktop launcher
- Auto-desktop after setup timeout
- Persistent writable VM disk installs
- `help`
- `apps`
- `setup`
- `save`
- `install`
- `net`
- `clear`
- `pwd`
- `ls [PATH]`
- `tree [PATH]`
- `cd DIR`
- `open PATH`
- `find NAME`
- `cat FILE`
- `stat PATH`
- `touch FILE`
- `new FILE`
- `write FILE TEXT`
- `append FILE TEXT`
- `mkdir DIR`
- `rm PATH`
- `cp SRC DST`
- `mv SRC DST`
- `desktop`
- `launcher`
- `files`
- `notes`
- `journal`
- `calc [EXPR]`
- `edit FILE`
- `installer`
- `store`
- `pkglist`
- `installpkg ID`
- `removepkg ID`
- `baremetal`
- `settings`
- `system`
- `quickstart`
- `welcome`
- `guide`
- `about`
- `reboot`

## SNSX App Store

The Store app is now part of the desktop and shell. It manages:

- Core packages such as the Browser, Snake, and the built-in `82540EM` driver path
- Optional kits such as the bare-metal install kit
- Driver-pack scaffolding for future Wi-Fi, Bluetooth, and printer support

Installed package assets are written into `/apps`, `/games`, `/drivers`, or `/kits` inside the SNSX filesystem.

## File Manager App

Desktop path:

```text
Type `1` at the desktop, or press Enter for the shell and run `files`
```

Launch:

```text
files
```

Inside the file manager:

- `open N` opens a file or enters a directory
- `edit N` edits a text file
- `cd N` enters a directory
- `del N` removes a file or empty directory
- `mkdir NAME` creates a directory
- `new NAME` creates a file and opens it
- `ren N NAME` renames an entry
- `copy N NAME` copies a file
- `up` goes to the parent directory
- `quit` returns to the desktop

## Text Editor App

Desktop path:

```text
Type `2` at the desktop, or press Enter for the shell and run `edit /home/NOTES.TXT`
```

Launch:

```text
edit /home/NOTES.TXT
```

Commands inside the editor:

- `.save` saves and exits
- `.quit` exits without saving
- `.show` shows the current buffer
- `.clear` clears the buffer
- `.help` shows editor commands

## Calculator App

Desktop path:

```text
Type `4` at the desktop, or press Enter for the shell and run `calc`
```

Examples:

```text
calc 12*(7-3)
calc
```

## Installer App

Desktop path:

```text
Type `9` at the desktop, or press Enter for the shell and run `install`
```

Inside the installer:

- `install` installs `BOOTAA64.EFI` to the writable SNSX disk
- `save` writes the current RAM filesystem snapshot
- `status` shows persistence state
- `guide` opens install docs
- `back` returns to the desktop

## Network App

Desktop path:

```text
Type `11` at the desktop, then choose `network`, or press Enter for the shell and run `network`
```

The current ARM64 build now includes a working native `82540EM` VirtualBox Ethernet path. In the shell, use:

```text
netconnect
ip
dns example.com
http http://example.com/
http https://example.com/
```

That gives SNSX a DHCP lease, IPv4 address, gateway, DNS server, and browser fetches over the virtual wired LAN path. `http://` uses the native SNSX browser stack directly. `https://` uses the optional host bridge started with `make https-bridge`, which is the current VirtualBox path until native guest-side TLS lands in SNSX itself.

If SNSX Browser says it could not reach the HTTPS bridge, restart the host helper:

```text
make https-bridge-stop
make https-bridge
```

## Settings App

Desktop path:

```text
Type `10` at the desktop, or press Enter for the shell and run `settings`
```

Inside Settings:

- `theme ocean|sunrise|slate|emerald`
- `autosave on|off|toggle`
- `save`
- `back`

## System Center

Desktop path:

```text
Type `11` at the desktop, or press Enter for the shell and run `system`
```

Inside System Center:

- `devices`
- `network`
- `about`
- `save`
- `back`

## Project Layout

```text
boot/           x86 boot code
kernel/         x86 kernel
drivers/        x86 low-level drivers
arm64/          Apple Silicon VirtualBox edition source
include/        shared freestanding headers
lib/            string/memory helpers shared by both builds
iso/            x86 GRUB ISO staging
iso-arm64/      ARM64 EFI ISO staging
rootfs/         x86 initrd content
tools/          host-side image tools
```

## Verified Artifacts

The following now build successfully:

- `SNSX.iso`
- `SNSX.img`
- `SNSX-VBox-ARM64.iso`

The ARM64 VirtualBox edition was also boot-tested under AArch64 UEFI in QEMU before being handed off as the VirtualBox target.
## Developer & Note
- Satya Narayan Sahu
- This OS version and relational versions and projects are totally owned by Satya Narayan Sahu and doesn't allows other to download and copy and mofidy and go for any commercial release, its only for educational purpose and the user/developer can only copy and do anything after he/she gives the fees and take license to work on it from Satya Narayan Sahu.
