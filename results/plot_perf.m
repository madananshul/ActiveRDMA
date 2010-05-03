times = [nfs_ab_mean(7), arfs_rdma_ab_mean(7), arfs_active_ab_mean(7) ;
nfs_stream_read_mean(7), arfs_rdma_stream_read_mean(7), arfs_active_stream_read_mean(7) ;
nfs_stream_write_mean(7), arfs_rdma_stream_write_mean(7), arfs_active_stream_write_mean(7) ]

stddev = [nfs_ab_stddev(7), arfs_rdma_ab_stddev(7), arfs_active_ab_stddev(7) ;
nfs_stream_read_stddev(7), arfs_rdma_stream_read_stddev(7), arfs_active_stream_read_stddev(7) ;
nfs_stream_write_stddev(7), arfs_rdma_stream_write_stddev(7), arfs_active_stream_write_stddev(7) ]

barweb(1e-9*times, 1.96e-9*stddev, [], {'AB', 'Stream-Read', 'Stream-Write'}, 'Wall-clock Performance', 'FS', 'Runtime (s)', [], [], {'NFS', 'ARFS-rdma', 'ARFS-active'}, [], [])
