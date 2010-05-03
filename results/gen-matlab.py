#!/usr/bin/python

import math

stats = 7

def pad(l):
    if len(l) < stats:
        l += [0 for i in range(stats - len(l))]
    return l

def read_file(f):
    lines = open(f).readlines()
    before = lines[1]
    after = lines[3]

    return pad(map(lambda x,y:x-y, map(int, after.split()), map(int, before.split())))

def read_files(dir):

    dat = []
    for i in range(10):
        dat.append(read_file(dir + '/t%d' % i))

    print dat[0]

    means = []
    stddev = []
    for i in range(stats):
        avg = 0.0
        for j in range(10):
           avg += float(dat[j][i])
        avg /= 10.0
        dev = 0.0
        for j in range(10):
            dev += ( (float(dat[j][i]) - avg) * (float(dat[j][i]) - avg) )
        dev /= 9.0
        dev = math.sqrt(dev)

        means.append(avg)
        stddev.append(dev)

    return means, stddev

def output(exp, stat_name):
    means, stddev = read_files(exp)
    means = map(str, means)
    stddev = map(str, stddev)
    print '%s_mean = [ %s ]; %s_stddev = [ %s ];' % (stat_name, ','.join(means), stat_name, ','.join(stddev))

for x in ['nfs', 'arfs/active', 'arfs/rdma']:
    for y in ['stream_read', 'stream_write', 'ab', 'find', 'grep', 'active_find', 'active_grep', 'scale/5']:
        if y.startswith('active') and not x.startswith('arfs'): continue
        dir_name = x + '/' + y
        stat_name = dir_name.replace('/', '_')
        output(dir_name, stat_name)
