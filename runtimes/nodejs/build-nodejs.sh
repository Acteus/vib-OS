#!/bin/bash
#
# UnixOS - Node.js 20 LTS Build Script
#
# Downloads and builds Node.js 20 LTS for ARM64 UnixOS
#

set -e

NODE_VERSION="20.10.0"
NODE_URL="https://nodejs.org/dist/v${NODE_VERSION}/node-v${NODE_VERSION}.tar.gz"

BUILD_DIR="$(pwd)/build/nodejs"
SYSROOT="$(pwd)/build/sysroot"
PREFIX="/usr"

echo "======================================="
echo " Building Node.js v${NODE_VERSION}"
echo "======================================="
echo ""

# Create directories
mkdir -p "${BUILD_DIR}"
mkdir -p "${SYSROOT}${PREFIX}"

# Download Node.js if not present
if [ ! -f "${BUILD_DIR}/node-v${NODE_VERSION}.tar.gz" ]; then
    echo "[DOWNLOAD] Node.js v${NODE_VERSION}"
    cd "${BUILD_DIR}"
    curl -LO "${NODE_URL}"
fi

# Extract if needed
if [ ! -d "${BUILD_DIR}/node-v${NODE_VERSION}" ]; then
    echo "[EXTRACT] Node.js source"
    cd "${BUILD_DIR}"
    tar xf "node-v${NODE_VERSION}.tar.gz"
fi

cd "${BUILD_DIR}/node-v${NODE_VERSION}"

echo "[CONFIGURE] Node.js"

# Note: This is a simplified script. Real cross-compilation requires:
# 1. Cross-compile libuv for target
# 2. Cross-compile V8 for ARM64
# 3. Configure Node.js with proper sysroot

cat << 'EOF'

=== Cross-Compilation Notes ===

For actual cross-compilation of Node.js to UnixOS:

1. Build dependencies:
   - libuv (async I/O library)
   - V8 JavaScript engine (for ARM64)
   - ICU (internationalization)
   - OpenSSL
   - zlib

2. Configure Node.js:
   ./configure \
       --dest-cpu=arm64 \
       --dest-os=linux \
       --cross-compiling \
       --prefix=/usr \
       --without-npm \
       --with-intl=none \
       --shared

3. Build:
   CC_target=aarch64-linux-musl-gcc \
   CXX_target=aarch64-linux-musl-g++ \
   make -j$(nproc)

4. Install to sysroot:
   make DESTDIR=${SYSROOT} install

Required for UnixOS:
- libuv port (depends on our kernel)
- V8 ARM64 support (upstream)
- File system compatibility
- Network stack (TCP/UDP)

EOF

echo ""
echo "[INFO] Node.js scaffold created"
echo "[INFO] Full build requires complete network stack"
