SYSTEM_HEADER_PROJECTS="../libc ../kernel ../userspace"
PROJECTS="../libc ../kernel ../userspace"

export MAKE=${MAKE:-make}
export HOST=${HOST:-$(./default-host.sh)}

export AR=${HOST}-ar
export AS=${HOST}-as
export CC=${HOST}-gcc

export PREFIX=/usr
export EXEC_PREFIX=$PREFIX
export BOOTDIR=/boot
export LIBDIR=$EXEC_PREFIX/lib
export INCLUDEDIR=$PREFIX/include

export FS_ROOT=fs-root/
export BINARY_PREFIX=/usr/bin

ISO="os.iso"
export ISO


CFLAGS="${CFLAGS} -O2 -g"
export CFLAGS
export CPPFLAGS=''

# Configure the cross-compiler to use the desired system root.
export SYSROOT="$(pwd)/sysroot"
export CC="$CC --sysroot=$SYSROOT"

export  QEMU_FLAGS=" -cdrom os.iso
-m 512M
-machine pc
-d page
-D log.txt
-boot d 
-drive file=qemu.img,format=raw,if=ide,index=0"

# Work around that the -elf gcc targets doesn't have a system include directory
# because it was configured with --without-headers rather than --with-sysroot.
if echo "$HOST" | grep -Eq -- '-elf($|-)'; then
  export CC="$CC -isystem=$INCLUDEDIR"
fi
