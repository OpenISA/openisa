cmd_/tools/include/linux/byteorder/.install := /bin/bash scripts/headers_install.sh /tools/include/linux/byteorder   /l/home/rafael/disco2/rafael/archc/openisa/cross/linux-3.10.14/include/uapi/linux/byteorder/big_endian.h   /l/home/rafael/disco2/rafael/archc/openisa/cross/linux-3.10.14/include/uapi/linux/byteorder/little_endian.h ; for F in ; do echo "\#include <asm-generic/$$F>" > /tools/include/linux/byteorder/$$F; done; touch /tools/include/linux/byteorder/.install