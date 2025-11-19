SYSTEM_HEADER_PROJECTS="../libc ../kernel ../userspace"
PROJECTS="../libc ../kernel ../userspace"

export MAKE=${MAKE:-make}
export HOST=${HOST:-$(./default-host.sh)}

COMPILER_PREFIX=$(./compiler-arch.sh $HOST) 

export AR=${COMPILER_PREFIX}-ar
export AS=${COMPILER_PREFIX}-as
export CC=${COMPILER_PREFIX}-gcc

export PREFIX=/usr
export EXEC_PREFIX=$PREFIX
export BOOTDIR=/boot
export LIBDIR=$EXEC_PREFIX/lib
export INCLUDEDIR=$PREFIX/include

export FS_ROOT=fs-root/
export BINARY_PREFIX=/usr/bin

ISO="os.iso"
export ISO




export ARCH=$(./default-host.sh)
CFLAGS="-DMYOS1234 ${CFLAGS} -Werror -Wno-unused-parameter -Wno-unused-label -Wno-unused-variable -Wno-error=type-limits -O0 -g -isystem=/usr/include -nostdlib $(./arch-flags.sh $ARCH) -DCONFIG_$ARCH " 
export CFLAGS
export CPPFLAGS=''

export QEMU_COMMAND=qemu-system-$(./qemu-arch.sh $ARCH)

# Configure the cross-compiler to use the desired system root.
export SYSROOT="$(pwd)/sysroot"
export CC="$CC --sysroot=$SYSROOT"


QEMU_FLAGS=" -cdrom os.iso
-m 512M
-machine pc
-d int
-D log.txt
-boot d 
-drive file=qemu.img,format=raw,if=ide,index=0"

# if [ $HOST =  "x86_64" ]; then

# 	cp -f /run/libvirt/nix-ovmf/OVMF_CODE.fd . 
# 	cp -f /run/libvirt/nix-ovmf/OVMF_VARS.fd . 
	
# 	QEMU_FLAGS="$QEMU_FLAGS -cpu qemu64
# 	-drive if=pflash,format=raw,unit=0,file=OVMF_CODE.fd,readonly=on 
#   	-drive if=pflash,format=raw,unit=1,file=OVMF_VARS.fd 
# "


# fi

export QEMU_FLAGS

# Work around that the -elf gcc targets doesn't have a system include directory
# because it was configured with --without-headers rather than --with-sysroot.
if echo "$HOST" | grep -Eq -- '-elf($|-)'; then
  export CC="$CC -isystem=$INCLUDEDIR"
fi
