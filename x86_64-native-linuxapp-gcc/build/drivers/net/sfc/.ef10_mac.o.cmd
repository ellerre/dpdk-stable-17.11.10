cmd_ef10_mac.o = gcc -Wp,-MD,./.ef10_mac.o.d.tmp  -m64 -pthread -fPIC  -march=native -mno-avx512f -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 -DRTE_MACHINE_CPUFLAG_AES -DRTE_MACHINE_CPUFLAG_PCLMULQDQ -DRTE_MACHINE_CPUFLAG_AVX -DRTE_MACHINE_CPUFLAG_RDRAND -DRTE_MACHINE_CPUFLAG_FSGSBASE -DRTE_MACHINE_CPUFLAG_F16C -DRTE_MACHINE_CPUFLAG_AVX2  -I/users/lr499/dpdk-stable-17.11.10/x86_64-native-linuxapp-gcc/include -include /users/lr499/dpdk-stable-17.11.10/x86_64-native-linuxapp-gcc/include/rte_config.h -I/users/lr499/dpdk-stable-17.11.10/drivers/net/sfc/base/ -I/users/lr499/dpdk-stable-17.11.10/drivers/net/sfc -O3 -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wpointer-arith -Wcast-align -Wnested-externs -Wcast-qual -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings -Wno-address-of-packed-member -Wimplicit-fallthrough=2 -Wno-format-truncation -Wno-strict-aliasing -Wextra -Wdisabled-optimization -Waggregate-return -Wnested-externs -Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable -Wno-empty-body -Wno-unused-but-set-variable   -o ef10_mac.o -c /users/lr499/dpdk-stable-17.11.10/drivers/net/sfc/base/ef10_mac.c 
