cmd_rte_ipv6_fragmentation.o = gcc -Wp,-MD,./.rte_ipv6_fragmentation.o.d.tmp  -m64 -pthread -fPIC  -march=native -mno-avx512f -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 -DRTE_MACHINE_CPUFLAG_AES -DRTE_MACHINE_CPUFLAG_PCLMULQDQ -DRTE_MACHINE_CPUFLAG_AVX -DRTE_MACHINE_CPUFLAG_RDRAND -DRTE_MACHINE_CPUFLAG_FSGSBASE -DRTE_MACHINE_CPUFLAG_F16C -DRTE_MACHINE_CPUFLAG_AVX2  -I/users/lr499/dpdk-stable-17.11.10/x86_64-native-linuxapp-gcc/include -include /users/lr499/dpdk-stable-17.11.10/x86_64-native-linuxapp-gcc/include/rte_config.h -O3 -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wpointer-arith -Wcast-align -Wnested-externs -Wcast-qual -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings -Wno-address-of-packed-member -Wimplicit-fallthrough=2 -Wno-format-truncation -I/users/lr499/dpdk-stable-17.11.10/lib/librte_ip_frag    -o rte_ipv6_fragmentation.o -c /users/lr499/dpdk-stable-17.11.10/lib/librte_ip_frag/rte_ipv6_fragmentation.c 
