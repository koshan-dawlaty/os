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


CFLAGS="${CFLAGS} -O1 -g  -mcmodel=kernel -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -DCONFIG_x86_64" 
export CFLAGS
export CPPFLAGS=''

# Configure the cross-compiler to use the desired system root.
export SYSROOT="$(pwd)/sysroot"
export CC="$CC --sysroot=$SYSROOT"


QEMU_FLAGS=" -cdrom os.iso
-m 512M
-machine pc
-d mmu
-D log.txt
-boot d 
-drive file=qemu.img,format=raw,if=ide,index=0"

if [ $HOST =  "x86_64" ]; then

	# TODO: probably not secure, and nixos only
	cp -f /run/libvirt/nix-ovmf/OVMF_CODE.fd . 
	cp -f /run/libvirt/nix-ovmf/OVMF_VARS.fd . 
	
	QEMU_FLAGS="$QEMU_FLAGS -cpu qemu64
	-drive if=pflash,format=raw,unit=0,file=OVMF_CODE.fd,readonly=on 
  	-drive if=pflash,format=raw,unit=1,file=OVMF_VARS.fd 
"


fi

export QEMU_FLAGS

# Work around that the -elf gcc targets doesn't have a system include directory
# because it was configured with --without-headers rather than --with-sysroot.
if echo "$HOST" | grep -Eq -- '-elf($|-)'; then
  export CC="$CC -isystem=$INCLUDEDIR"
fi
