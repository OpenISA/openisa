cmd_/tools/include/linux/raid/.install := /bin/bash scripts/headers_install.sh /tools/include/linux/raid   /l/home/rafael/disco2/rafael/archc/openisa/cross/linux-3.10.14/include/uapi/linux/raid/md_p.h   /l/home/rafael/disco2/rafael/archc/openisa/cross/linux-3.10.14/include/uapi/linux/raid/md_u.h ; for F in ; do echo "\#include <asm-generic/$$F>" > /tools/include/linux/raid/$$F; done; touch /tools/include/linux/raid/.install