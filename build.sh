#!/bin/bash

ROOT=$(pwd)
MAKE_OPTS=${MAKE_OPTS:-}

function check_error {
    [ $? -ne 0 ] && {
        echo "Command failed :-("
        exit
    }
}

function install_pkgs {
    sudo apt-get update &&
    sudo apt-get install -y \
        build-essential \
        bzip2 \
        cmake \
        gawk \
        git \
        gnuplot-nox \
        libc6-dev-i386 \
        lib32z1 \
        libunwind8:i386 \
        linux-perf-4.9:i386 \
        ninja-build \
        python \
        r-base \
        time \
        wget || check_error
}

if [ "$1" = install_pkgs ]; then
    install_pkgs
    exit
fi

# Compile binutils for openisa, if it was not built already
[ ! -f ${ROOT}/oi-toolchain/bin/mipsel-unknown-linux-gnu-ld ] && {
    cd linker-oi || check_error
    if [ ! -d binutils-2.23.2 ]; then
        wget http://ftp.gnu.org/gnu/binutils/binutils-2.23.2.tar.bz2 || check_error
        tar xjvf binutils-2.23.2.tar.bz2 || check_error
        git reset --hard || check_error
    fi
    mkdir -p build || check_error
    cd build || check_error
    ../binutils-2.23.2/configure --prefix=${ROOT}/oi-toolchain \
                                 --disable-nls --enable-shared --disable-multilib \
                                 --target=mipsel-unknown-linux-gnu --disable-werror || check_error
    make MAKEINFO=true -j20 || check_error
    make install MAKEINFO=true || check_error
}

# Clone LLVM openisa repo
[ ! -d ${ROOT}/llvm-openisa ] && {
    cd ${ROOT} || check_error
    git clone --recursive https://github.com/openisa/llvm-openisa -b v0.4 --depth 1 || check_error
}

# Build OpenISA cross compiling infrastructure and static binary translator
[ ! -f ${ROOT}/obj/bin/clang ] && {
    cd ${ROOT} || check_error
    mkdir -p obj || check_error
    cd obj || check_error
    if [ ! -f build.ninja ]; then
        cmake ../llvm-openisa -DLLVM_TARGETS_TO_BUILD="Mips;X86;ARM" \
              -DCMAKE_INSTALL_PREFIX=${ROOT}/oi-toolchain \
              -DLLVM_DEFAULT_TARGET_TRIPLE="mipsel-unknown-linux-gnu" \
              -DCLANG_VENDOR="OpenISA Tools" -GNinja || check_error
    fi
    ninja $MAKE_OPTS || check_error
}

# Install OpenISA cross compiler
[ ! -f ${ROOT}/oi-toolchain/bin/clang ] && {
    cd ${ROOT}/obj || check_error
    ninja install || check_error
}

# Use OpenISA cross compiler to build newlib (libc)
export PATH=${ROOT}/oi-toolchain/bin:$PATH
[ ! -f ${ROOT}/oi-toolchain/oi-elf/lib/libc.a ] && {
    cd ${ROOT}/newlib || check_error
    mkdir -p build || check_error
    cd build
    CC_FOR_TARGET="clang"\
                 CXX_FOR_TARGET="clang++"\
                 AR_FOR_TARGET="mipsel-unknown-linux-gnu-ar"\
                 AS_FOR_TARGET="mipsel-unknown-linux-gnu-as"\
                 LD_FOR_TARGET="mipsel-unknown-linux-gnu-ld"\
                 NM_FOR_TARGET="mipsel-unknown-linux-gnu-nm"\
                 OBJDUMP_FOR_TARGET="mipsel-unknown-linux-gnu-objdump"\
                 RANLIB_FOR_TARGET="mipsel-unknown-linux-gnu-ranlib"\
                 READELF_FOR_TARGET="mipsel-unknown-linux-gnu-readelf"\
                 STRIP_FOR_TARGET="mipsel-unknown-linux-gnu-strip"\
                 ../newlib-2.1.0/configure --without-docdir --prefix=${ROOT}/oi-toolchain -target=oi-elf || check_error
    echo "MAKEINFO = :" >> Makefile
    make || check_error
    make install || check_error
    cp  -v ../newlib-2.1.0/libgloss/libnosys/ac_link.ld ${ROOT}/oi-toolchain/oi-elf/lib || check_error
    cd ${ROOT}/oi-toolchain/oi-elf/lib || check_error
    clang -c ${ROOT}/newlib/newlib-2.1.0/libgloss/libnosys/ac_start.S \
          -o ac_start.o || check_error
}

# Create a symlink to make the static binary translator work with the path to
# OpenISA libc/kernel headers
if [ ! -L /tools ]; then
    sudo ln -s ${ROOT}/sbt-sysheaders /tools || check_error
fi

# TODO
# cd ${ROOT}/spec
# Download SPEC separately as we are not allowed to distribute it
# wget http://www.lsc.ic.unicamp.br/~rafaelauler/files.nobackup/openisa-tests.tar.bz2 || check_error

# TODO
# Download an ARM cross-compiler
# wget http://www.lsc.ic.unicamp.br/~rafaelauler/files.nobackup/arm-compiler.tar.bz2 || check_error

# TODO
# Install SPEC sources and ARM cross-compiler
# tar xvf openisa-tests.tar.bz2 || check_error
# pushd /
# tar xvf ${ROOT}/openisa/spec/arm-compiler.tar.bz2 || check_error
# popd
# rm openisa-tests.tar.bz2
# rm arm-compiler.tar.bz2

# Create a file to configure environmental variables - the user just needs to
# source "env.sh"
# TODO: add ARM cross-compiler to PATH
echo export PATH=${ROOT}/oi-toolchain/bin:\$PATH > ${ROOT}/env.sh
echo "echo -1 | sudo tee /proc/sys/kernel/perf_event_paranoid" >> \
${ROOT}/env.sh

# Image is almost ready. If you do not want to build all benchmarks, comment the
# lines below and it should be fine (you can run the lines below later).
[ ! -f ${ROOT}/spec/testes-x86/bin/susan-locals ] && {
    cd ${ROOT}/mibench || check_error
    ./build_all_x86.sh || check_error
    mkdir -p ${ROOT}/spec/testes-x86/bin
    cp -v testes-x86/bin/* ${ROOT}/spec/testes-x86/bin || check_error
}

# TODO: build SPEC
# cd ${ROOT}/spec || check_error
# ./build_all_x86.sh || check_error
echo "Finished. Tools installed in ${ROOT}/oi-toolchain and binaries in ${ROOT}/oi-toolchain/bin"

