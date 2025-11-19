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

### Apple Silicon (macOS ARM64)

**Native build on M1/M2/M3 Mac:**

```bash
# Install dependencies via Homebrew
brew install fltk pixman gnutls libjpeg-turbo cmake

# Build TigerVNC
mkdir build
cd build
cmake ..
make
```

**Cross-compile from Intel Mac to Apple Silicon:**

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install ARM64 Homebrew and dependencies
arch -arm64 /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
arch -arm64 /opt/homebrew/bin/brew install fltk pixman gnutls libjpeg-turbo

# Build for ARM64
mkdir build-arm64
cd build-arm64
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchains/apple-arm64.cmake ..
make
```

**Universal Binary (Intel + Apple Silicon):**

```bash
# Requires dependencies built for both architectures
mkdir build-universal
cd build-universal
cmake -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" ..
make
```

### Cross-Compilation from x86_64 Linux

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
- **Pixman**: ARM64-optimized pixel manipulation routines with NEON acceleration
- **GnuTLS**: Hardware-accelerated cryptography on ARM64
- **FLTK**: Native ARM64 GUI rendering

### Apple Silicon Specific

Apple Silicon (M1/M2/M3) provides additional performance benefits:

- **Unified Memory**: Reduced memory latency for graphics operations
- **Hardware AES**: M-series chips include AES encryption acceleration
- **Advanced SIMD**: Apple's NEON implementation with enhanced performance
- **Rosetta 2**: x86_64 TigerVNC binaries run efficiently via Rosetta 2 if needed

## Tested Platforms

TigerVNC ARM64 builds have been tested on:

### Linux ARM64
- AWS Graviton2/Graviton3 instances
- Raspberry Pi 4/5 (4GB/8GB)
- NVIDIA Jetson platforms (Nano, Xavier, Orin)
- ARM64 Linux VMs (KVM, VMware)
- Oracle Cloud ARM instances

### Apple Silicon (macOS)
- MacBook Air M1/M2/M3
- MacBook Pro M1/M2/M3 (Pro/Max/Ultra)
- Mac Mini M1/M2
- Mac Studio M1/M2 (Max/Ultra)
- iMac M1/M3

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

To test ARM64 binaries on x86_64 Linux systems, use QEMU:

```bash
sudo apt-get install qemu-user qemu-user-static
qemu-aarch64-static -L /usr/aarch64-linux-gnu ./vncviewer
```

### macOS Specific Issues

**Homebrew architecture mismatch:**

If CMake finds the wrong architecture libraries, specify the pkg-config path:

```bash
# For ARM64 build, use ARM64 Homebrew
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig"

# For x86_64 build, use x86_64 Homebrew
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig"
```

**Checking binary architecture:**

Verify your built binary architecture:

```bash
file ./vncviewer
# Should show: Mach-O 64-bit executable arm64

# Or for universal binary:
lipo -info ./vncviewer
# Should show: Architectures in the fat file: ./vncviewer are: x86_64 arm64
```

**Code signing on macOS:**

macOS may require code signing for GUI applications:

```bash
codesign -s - ./vncviewer
```

## More Information

For detailed build instructions, see [BUILDING.txt](BUILDING.txt).
