# ARM64 Support for TigerVNC

TigerVNC has full support for ARM64 (aarch64) architecture. The codebase is architecturally portable with no platform-specific assembly code.

## Quick Start

### Native ARM64 Build

If you're building on an ARM64 system (e.g., Raspberry Pi 4, AWS Graviton, Apple Silicon):

```bash
mkdir build
cd build
cmake ..
make
sudo make install
```

### Cross-Compilation from x86_64

#### 1. Install Cross-Compilation Toolchain

**Debian/Ubuntu:**
```bash
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc-aarch64-linux-gnu gcc-c++-aarch64-linux-gnu
```

#### 2. Install ARM64 Dependencies (Debian/Ubuntu)

```bash
sudo dpkg --add-architecture arm64
sudo apt-get update
sudo apt-get install \
    zlib1g-dev:arm64 \
    libpixman-1-dev:arm64 \
    libjpeg-turbo8-dev:arm64 \
    libgnutls28-dev:arm64 \
    libfltk1.3-dev:arm64 \
    libpam0g-dev:arm64 \
    libx11-dev:arm64 \
    libxtst-dev:arm64 \
    libxext-dev:arm64
```

#### 3. Build with Toolchain File

```bash
mkdir build-arm64
cd build-arm64
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchains/aarch64-linux-gnu.cmake ..
make
```

## Performance Notes

All major dependencies have ARM64 optimizations:

- **libjpeg-turbo**: Uses ARM NEON SIMD instructions for fast JPEG encoding/decoding
- **Pixman**: ARM64-optimized pixel manipulation routines
- **GnuTLS**: Hardware-accelerated cryptography on ARM64
- **FLTK**: Native ARM64 GUI rendering

## Tested Platforms

TigerVNC ARM64 builds have been tested on:

- AWS Graviton2/Graviton3 instances
- Raspberry Pi 4 (4GB/8GB)
- NVIDIA Jetson platforms
- Apple M1/M2 (via macOS builds)
- ARM64 Linux VMs

## Troubleshooting

### Missing Cross-Compiler

If you get "CMAKE_C_COMPILER not found" errors, ensure the cross-compiler is installed:

```bash
which aarch64-linux-gnu-gcc
```

### Library Dependencies

If CMake can't find ARM64 libraries, verify they're installed:

```bash
dpkg -l | grep :arm64
```

### Running ARM64 Binaries on x86_64

To test ARM64 binaries on x86_64 systems, use QEMU:

```bash
sudo apt-get install qemu-user qemu-user-static
qemu-aarch64-static -L /usr/aarch64-linux-gnu ./vncviewer
```

## More Information

For detailed build instructions, see [BUILDING.txt](BUILDING.txt).
