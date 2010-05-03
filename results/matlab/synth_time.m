ns_to_ms = 1e-6;

rdma_cpu = ns_to_ms*arfs_rdma_ab_mean(1); rdma_cpu_err = ns_to_ms*arfs_rdma_ab_stddev(1)*1.96;
active_cpu = ns_to_ms*arfs_active_ab_mean(1); active_cpu_err = ns_to_ms*arfs_active_ab_stddev(1)*1.96;
rdma_pkts = arfs_rdma_ab_mean(3); rdma_pkts_err = arfs_rdma_ab_stddev(3)*1.96;
active_pkts = arfs_active_ab_mean(3); active_pkts_err = arfs_active_ab_stddev(3)*1.96;

latencies = [1 5 10 20 40 60 80 100];

rdma_synth_time = rdma_cpu + rdma_pkts*latencies;
rdma_synth_time_err = sqrt(rdma_cpu_err^2 + (latencies*rdma_pkts_err^2));
active_synth_time = active_cpu + active_pkts*latencies;
active_synth_time_err = sqrt(active_cpu_err^2 + (latencies*active_pkts_err^2));

plot(latencies, 1e-3*rdma_synth_time, latencies, 1e-3*active_synth_time);
%errorbar(latencies, rdma_synth_time, rdma_synth_time_err);
%errorbar(latencies, active_synth_time, active_synth_time_err);
legend('RDMA', 'Active');
title('Projected Runtime: Active vs. RDMA')
xlabel('Roundtrip Latency (ms)')
ylabel('Runtime (s)')

print('-dpdf', 'synth_time.pdf')