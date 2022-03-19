cmd_dpdk-test-crypto-perf = gcc -o dpdk-test-crypto-perf -m64 -pthread -fPIC  -march=native -mno-avx512f -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 -DRTE_MACHINE_CPUFLAG_AES -DRTE_MACHINE_CPUFLAG_PCLMULQDQ -DRTE_MACHINE_CPUFLAG_AVX -DRTE_MACHINE_CPUFLAG_RDRAND -DRTE_MACHINE_CPUFLAG_FSGSBASE -DRTE_MACHINE_CPUFLAG_F16C -DRTE_MACHINE_CPUFLAG_AVX2  -I/users/lr499/dpdk-stable-17.11.10/x86_64-native-linuxapp-gcc/include -include /users/lr499/dpdk-stable-17.11.10/x86_64-native-linuxapp-gcc/include/rte_config.h -W -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wpointer-arith -Wcast-align -Wnested-externs -Wcast-qual -Wformat-nonliteral -Wformat-security -Wundef -Wwrite-strings -Wno-address-of-packed-member -Wimplicit-fallthrough=2 -Wno-format-truncation  main.o cperf_ops.o cperf_options_parsing.o cperf_test_vectors.o cperf_test_throughput.o cperf_test_latency.o cperf_test_pmd_cyclecount.o cperf_test_verify.o cperf_test_vector_parsing.o cperf_test_common.o -Wl,-lrte_pmd_crypto_scheduler -L/users/lr499/dpdk-stable-17.11.10/x86_64-native-linuxapp-gcc/lib -Wl,-lrte_flow_classify -Wl,-lrte_pipeline -Wl,-lrte_table -Wl,-lrte_port -Wl,-lrte_pdump -Wl,-lrte_distributor -Wl,-lrte_ip_frag -Wl,-lrte_gro -Wl,-lrte_gso -Wl,-lrte_meter -Wl,-lrte_lpm -Wl,--whole-archive -Wl,-lrte_acl -Wl,--no-whole-archive -Wl,-lrte_jobstats -Wl,-lrte_metrics -Wl,-lrte_bitratestats -Wl,-lrte_latencystats -Wl,-lrte_power -Wl,-lrte_timer -Wl,-lrte_efd -Wl,--whole-archive -Wl,-lrte_cfgfile -Wl,-lrte_hash -Wl,-lrte_member -Wl,-lrte_vhost -Wl,-lrte_kvargs -Wl,-lrte_mbuf -Wl,-lrte_net -Wl,-lrte_ethdev -Wl,-lrte_cryptodev -Wl,-lrte_security -Wl,-lrte_eventdev -Wl,-lrte_mempool -Wl,-lrte_mempool_ring -Wl,-lrte_ring -Wl,-lrte_pci -Wl,-lrte_eal -Wl,-lrte_cmdline -Wl,-lrte_reorder -Wl,-lrte_sched -Wl,-lrte_kni -Wl,-lrte_bus_pci -Wl,-lrte_bus_vdev -Wl,--no-whole-archive -Wl,-lgcc_s -Wl,-ldl -Wl,-export-dynamic -Wl,-export-dynamic -Wl,-export-dynamic -L/users/lr499/dpdk-stable-17.11.10/x86_64-native-linuxapp-gcc/lib -Wl,--as-needed -Wl,-Map=dpdk-test-crypto-perf.map -Wl,--cref 
