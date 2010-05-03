times = [
nfs_grep_mean(7), arfs_rdma_grep_mean(7), arfs_active_grep_mean(7) ;
nfs_find_mean(7), arfs_rdma_find_mean(7), arfs_active_find_mean(7) ;
0, arfs_rdma_active_grep_mean(7), arfs_active_active_grep_mean(7) ;
0, arfs_rdma_active_find_mean(7), arfs_active_active_find_mean(7) ;
]

stddev = [
nfs_grep_stddev(7), arfs_rdma_grep_stddev(7), arfs_active_grep_stddev(7) ;
nfs_find_stddev(7), arfs_rdma_find_stddev(7), arfs_active_find_stddev(7) ;
0, arfs_rdma_active_grep_stddev(7), arfs_active_active_grep_stddev(7) ;
0, arfs_rdma_active_find_stddev(7), arfs_active_active_find_stddev(7) ;
]

barweb(1e-9*times, 1.96e-9*stddev, [], {'Grep', 'Find', 'Active-Grep', 'Active-Find'}, 'Wall-clock Performance', 'FS', 'Runtime (s)', [], [], {'NFS', 'ARFS-rdma', 'ARFS-active'}, [], [])

print('-dpdf', 'wallclock2.pdf')