#!/bin/bash

ROOT=$(pwd)/oitools


function check_error {
    [ $? -ne 0 ] && {
        echo "Command failed :-("
        exit
    }
}

mkdir -p $ROOT || check_error
cd $ROOT || check_error
[ ! -f ${ROOT}/oi-toolchain/bin/mipsel-unknown-linux-gnu-ld ] && {
    git clone https://github.com/rafaelauler/openisa -b v0.4 --depth 1 || check_error
    cd openisa/linker-oi || check_error
    wget http://ftp.gnu.org/gnu/binutils/binutils-2.23.2.tar.bz2 || check_error
    tar xjvf binutils-2.23.2.tar.bz2 || check_error
    git reset --hard || check_error
    mkdir -p build || check_error
    cd build || check_error
    ../binutils-2.23.2/configure --prefix=${ROOT}/oi-toolchain \
                                 --disable-nls --enable-shared --disable-multilib \
                                 --target=mipsel-unknown-linux-gnu --disable-werror || check_error
    make MAKEINFO=true -j20 || check_error
    make install MAKEINFO=true || check_error
}
[ ! -d ${ROOT}/llvm-openisa ] && {
    cd ${ROOT} || check_error
    git clone --recursive https://github.com/rafaelauler/llvm-openisa -b v0.4 --depth 1 || check_error
}
[ ! -f ${ROOT}/obj/bin/clang ] && {
    cd ${ROOT} || check_error
    mkdir -p obj || check_error
    cd obj || check_error
    cmake ../llvm-openisa -DLLVM_TARGETS_TO_BUILD="Mips;X86;ARM" \
          -DCMAKE_INSTALL_PREFIX=${ROOT}/oi-toolchain \
          -DLLVM_DEFAULT_TARGET_TRIPLE="mipsel-unknown-linux-gnu" \
          -DCLANG_VENDOR="OpenISA Tools" -GNinja || check_error
    ninja || check_error
}
[ ! -f ${ROOT}/oi-toolchain/bin/clang ] && {
    cd ${ROOT}/obj || check_error
    ninja install || check_error
    ninja clean
}
[ ! -f ${ROOT}/oi-toolchain/oi-elf/lib/libc.a ] && {
    cd ${ROOT}/oi-toolchain/bin || check_error
    export PATH=$(pwd):$PATH
    cd ${ROOT}/openisa/newlib || check_error
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
    clang -c ${ROOT}/openisa/newlib/newlib-2.1.0/libgloss/libnosys/ac_start.S \
          -o ac_start.o || check_error
}
ln -s ${ROOT}/openisa/sbt-sysheaders /tools || check_error
cd ${ROOT}/openisa/spec
wget http://www.lsc.ic.unicamp.br/~rafaelauler/files.nobackup/openisa-tests.tar.bz2 || check_error
wget http://www.lsc.ic.unicamp.br/~rafaelauler/files.nobackup/arm-compiler.tar.bz2 || check_error
tar xvf openisa-tests.tar.bz2 || check_error
pushd /
tar xvf ${ROOT}/openisa/spec/arm-compiler.tar.bz2 || check_error
popd
rm openisa-tests.tar.bz2
rm arm-compiler.tar.bz2
cd ${ROOT}/openisa/mibench || check_error
./build_all_x86.sh || check_error
cp -r testes-x86/bin ${ROOT}/openisa/spec/testes-x86/bin || check_error
cd ${ROOT}/openisa/spec || check_error
./build_all_x86.sh || check_error
echo export PATH=\$PATH:${ROOT}/oi-toolchain/bin:/home/rafaelauler/p/openisa/cross2/cross-tools/bin > ${ROOT}/../env.sh
echo "Finished. Tools installed in ${ROOT}/oi-toolchain and binaries in ${ROOT}/oi-toolchain/bin"


