cmd_/tools/include/linux/netfilter_arp/.install := /bin/bash scripts/headers_install.sh /tools/include/linux/netfilter_arp   /l/home/rafael/disco2/rafael/archc/openisa/cross/linux-3.10.14/include/uapi/linux/netfilter_arp/arp_tables.h   /l/home/rafael/disco2/rafael/archc/openisa/cross/linux-3.10.14/include/uapi/linux/netfilter_arp/arpt_mangle.h ; for F in ; do echo "\#include <asm-generic/$$F>" > /tools/include/linux/netfilter_arp/$$F; done; touch /tools/include/linux/netfilter_arp/.install