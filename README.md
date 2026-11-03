# 🛡️ AyhanOS

**AyhanOS** is a lightweight, secure, and fully static Linux-based operating system built from scratch. Designed for developers, security researchers, and users who value privacy and performance.

![AyhanOS Banner](https://via.placeholder.com/1200x300?text=AyhanOS+-+Security+First)

## 🚀 Key Features

- **AppArmor Security:** Root-proof sandboxing. Even `root` cannot bypass core security restrictions.
- **AyhanGuard Firewall:** A custom, fully static kernel-level firewall that blocks malicious ports (HTTP, Telnet, RDP, SMB, etc.) by default.
- **Thermal Protection:** Monitors CPU temperature. If it exceeds 65°C, the system restarts immediately to prevent hardware damage.
- **Ultra-Lightweight:** Runs efficiently on low-end hardware and virtual machines (optimized for QEMU).
- **Fully Static:** Core binaries (like AyhanGuard) are statically compiled. No external dependencies, no missing libraries.
- **Zero Telemetry:** No Google, no tracking, no bloatware.

## 🏗️ Architecture

- **Kernel:** Custom Linux 6.18.2 (Tiny Core based)
- **Init System:** Custom POSIX-compliant `init` script
- **Security:** AppArmor (Enforcing) + AyhanGuard (Kernel-level Raw Sockets)
- **Package Manager:** Custom (coming soon)

## 📥 Download & Installation

> **Note:** AyhanOS is currently in early development. Public releases will be available soon.

1. Download the latest `.img` or `.iso` from the **Releases** section.
2. Write the image to a USB drive or run it in **QEMU**:

```bash
qemu-system-x86_64 \
  -kernel vmlinuz-ultimate \
  -append "root=/dev/sda rw console=ttyS0 loglevel=3 security=apparmor apparmor=1 quiet" \
  -drive file=ayhan_disk_3g.img,format=raw,if=ide \
  -device virtio-net-pci,netdev=n0 \
  -netdev user,id=n0 \
  -device virtio-rng-pci \
  -accel tcg,thread=multi \
  -smp 4 \
  -nographic \
  -m 2048
```

🗄️ The AyhanOS Archive

We believe in preserving history. Every version of AyhanOS is permanently archived. Old files are protected as digital history and will never be removed.

🤝 Contributing

We welcome contributions! Please read our CONTRIBUTING.md for guidelines.

⚠️ Important Notice

AyhanOS is optimized for QEMU and virtualized environments. Installing on bare-metal hardware is possible but currently not recommended for production. We are working on expanding hardware support.

📜 License

AyhanOS is licensed under the GNU General Public License v3.0 (GPLv3). See the LICENSE file for details.

---

Built with ❤️ by AyhanDeveloper.
