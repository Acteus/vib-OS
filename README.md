# Vib-OS

**A Production-Grade ARM64 Operating System with GUI**

![Build Status](https://img.shields.io/badge/build-passing-brightgreen)
![Platform](https://img.shields.io/badge/platform-ARM64-blue)
![Apple Silicon](https://img.shields.io/badge/Apple%20Silicon-M1%20%7C%20M2%20%7C%20M3-orange)
![Raspberry Pi](https://img.shields.io/badge/Raspberry%20Pi-4%20%7C%205-red)
![Lines](https://img.shields.io/badge/lines-15.5k+-yellow)
![License](https://img.shields.io/badge/license-MIT-green)

```
        _  _         ___  ____  
 __   _(_)| |__     / _ \/ ___| 
 \ \ / / || '_ \   | | | \___ \ 
  \ V /| || |_) |  | |_| |___) |
   \_/ |_||_.__/    \___/|____/ 

Vib-OS v0.5.0 - ARM64 with Full GUI
```

## Overview

Vib-OS is a from-scratch, production-grade Unix-like operating system for ARM64. Built with **15,500+ lines** of C and Assembly:

- ✅ **Full GUI System** - Window manager, terminal, apps
- ✅ **APFS + ext4** - Read macOS and Linux filesystems
- ✅ **USB 3.x + Bluetooth** - XHCI and HCI drivers
- ✅ **Full TCP/IP Stack** - Ethernet, ARP, IP, ICMP, UDP, TCP, DNS
- ✅ **Apple Silicon + Raspberry Pi** - M1/M2/M3 and Pi 4/5
- ✅ **Python 3.12 + Node.js 20** - Runtime build scripts

## Supported Platforms

| Platform | Status | Notes |
|----------|--------|-------|
| **QEMU ARM64** | ✅ Tested | Primary development target |
| **VirtualBox** | ✅ Supported | Use `scripts/create-iso.sh` |
| **UTM (macOS)** | ✅ Supported | ARM64 native on Apple Silicon |
| **Apple M1/M2/M3** | ✅ Supported | All variants |
| **Raspberry Pi 4/5** | ✅ Supported | BCM2711/BCM2712 |
| **Generic ARM64** | ✅ Supported | Any ARMv8-A |

## Quick Start

### Build & Run in QEMU

```bash
git clone git@github.com:viralcode/vib-OS.git
cd vib-OS

make kernel
make run
```

### Run in VirtualBox

```bash
# Create bootable ISO
./scripts/create-iso.sh

# Then in VirtualBox:
# 1. Create new VM (Other/Unknown 64-bit)
# 2. Memory: 2GB+
# 3. Storage > Add ISO: build/vib-os.iso
# 4. Start!
```

### Run in UTM (macOS ARM64)

1. Download [UTM](https://mac.getutm.app/)
2. Create VM → Emulate → ARM64
3. Add kernel: `build/kernel/unixos.elf`
4. Start

## Features

### Kernel
| Component | Status |
|-----------|--------|
| MMU (4-level page tables) | ✅ |
| GIC v3 | ✅ |
| Scheduler + Signals | ✅ |
| Fork/Exec + ELF loading | ✅ |
| Boot config + Splash | ✅ |

### Filesystems
| FS | Status |
|----|--------|
| VFS + ramfs | ✅ |
| ext4 | ✅ |
| **APFS (read-only)** | ✅ |

### Networking
| Layer | Status |
|-------|--------|
| Ethernet/ARP/IP/ICMP | ✅ |
| UDP/TCP | ✅ |
| DNS resolver | ✅ |
| BSD Sockets | ✅ |

### Drivers
| Driver | Status |
|--------|--------|
| Apple AGX GPU | ✅ |
| Apple ANS NVMe | ✅ |
| Raspberry Pi 4/5 | ✅ |
| **USB 3.x (XHCI)** | ✅ |
| **Bluetooth (HCI)** | ✅ |

### GUI System
| Component | Status |
|-----------|--------|
| Window Manager | ✅ |
| Double-buffered Compositor | ✅ |
| VT100 Terminal | ✅ |
| Applications (Files, Editor, Settings) | ✅ |

### Runtime Ports
| Runtime | Status |
|---------|--------|
| musl libc | ✅ Build script |
| CPython 3.12 | ✅ Build script |
| Node.js 20 | ✅ Build script |

## Project Structure

```
Vib-OS/                     15,540+ lines
├── kernel/
│   ├── gui/               Window manager, terminal, apps
│   ├── net/               TCP/IP, DNS, sockets
│   ├── fs/                VFS, ramfs, ext4, APFS
│   └── ...
├── drivers/
│   ├── gpu/               Apple AGX
│   ├── nvme/              Apple ANS
│   ├── usb/               XHCI
│   ├── bluetooth/         HCI
│   └── platform/          Raspberry Pi
├── userspace/             Init, shell
├── libc/                  musl build
├── runtimes/              Python, Node.js builds
└── scripts/               ISO creation, utilities
```

## Build Commands

```bash
make clean          # Clean
make kernel         # Build kernel
make all            # Build everything
make run            # Run in QEMU
./scripts/create-iso.sh  # Create bootable ISO
```

## License

MIT License

---

**Vib-OS** - Built with ❤️ for ARM64
