cmd_/tools/include/linux/netfilter_ipv4/.install := /bin/bash scripts/headers_install.sh /tools/include/linux/netfilter_ipv4   /l/home/rafael/disco2/rafael/archc/openisa/cross/linux-3.10.14/include/uapi/linux/netfilter_ipv4/ip_tables.h   /l/home/rafael/disco2/rafael/archc/openisa/cross/linux-3.10.14/include/uapi/linux/netfilter_ipv4/ipt_CLUSTERIP.h   /l/home/rafael/disco2/rafael/archc/openisa/cross/linux-3.10.14/include/uapi/linux/netfilter_ipv4/ipt_ECN.h   /l/home/rafael/disco2/rafael/archc/openisa/cross/linux-3.10.14/include/uapi/linux/netfilter_ipv4/ipt_LOG.h   /l/home/rafael/disco2/rafael/archc/openisa/cross/linux-3.10.14/include/uapi/linux/netfilter_ipv4/ipt_REJECT.h   /l/home/rafael/disco2/rafael/archc/openisa/cross/linux-3.10.14/include/uapi/linux/netfilter_ipv4/ipt_TTL.h   /l/home/rafael/disco2/rafael/archc/openisa/cross/linux-3.10.14/include/uapi/linux/netfilter_ipv4/ipt_ULOG.h   /l/home/rafael/disco2/rafael/archc/openisa/cross/linux-3.10.14/include/uapi/linux/netfilter_ipv4/ipt_ah.h   /l/home/rafael/disco2/rafael/archc/openisa/cross/linux-3.10.14/include/uapi/linux/netfilter_ipv4/ipt_ecn.h   /l/home/rafael/disco2/rafael/archc/openisa/cross/linux-3.10.14/include/uapi/linux/netfilter_ipv4/ipt_ttl.h ; for F in ; do echo "\#include <asm-generic/$$F>" > /tools/include/linux/netfilter_ipv4/$$F; done; touch /tools/include/linux/netfilter_ipv4/.install