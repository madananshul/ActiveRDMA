nfs_write_rt_correction = 0.5

times = [nfs_ab_mean(3), arfs_rdma_ab_mean(3), arfs_active_ab_mean(3) ;
nfs_stream_read_mean(3), arfs_rdma_stream_read_mean(3), arfs_active_stream_read_mean(3) ;
nfs_write_rt_correction*nfs_stream_write_mean(3), arfs_rdma_stream_write_mean(3), arfs_active_stream_write_mean(3) ]

stddev = [nfs_ab_stddev(3), arfs_rdma_ab_stddev(3), arfs_active_ab_stddev(3) ;
nfs_stream_read_stddev(3), arfs_rdma_stream_read_stddev(3), arfs_active_stream_read_stddev(3) ;
nfs_write_rt_correction*nfs_stream_write_stddev(3), arfs_rdma_stream_write_stddev(3), arfs_active_stream_write_stddev(3) ]

barweb(times, 1.96*stddev, [], {'AB', 'Stream-Read (5MB)', 'Stream-Write (5MB)'}, 'Packet Roundtrips', 'FS', 'Packets', [], [], {'NFS', 'ARFS-rdma', 'ARFS-active'}, [], [])

print('-dpdf', 'rt.pdf')