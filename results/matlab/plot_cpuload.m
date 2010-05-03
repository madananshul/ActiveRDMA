clockspeed = 2; % instructions per ns

times = [nfs_ab_mean(2)/clockspeed, nfs_scale_5_mean(2)/clockspeed;
    arfs_rdma_ab_mean(1), arfs_rdma_scale_5_mean(1);
    arfs_active_ab_mean(1), arfs_active_scale_5_mean(1) ;

]


stddev = [nfs_ab_stddev(2)/clockspeed, nfs_scale_5_stddev(2)/clockspeed^2;
    arfs_rdma_ab_stddev(1), arfs_rdma_scale_5_stddev(1);
    arfs_active_ab_stddev(1), arfs_active_scale_5_stddev(1) ;
]

barweb(1e-9*times, 1.96e-9*stddev, [], {'NFS', 'ARFS-rdma', 'ARFS-active'}, 'CPU Load (AB)', 'FS', 'CPU time (s)', [], [], {'1 client', '5 clients'}, [], [])


print('-dpdf', 'cpuload.pdf')