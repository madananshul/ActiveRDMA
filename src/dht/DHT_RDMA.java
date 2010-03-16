package dht;

// arg is index to null-terminated key in memory.
// hash table is at address N, has N entries, each entry is 
// <next> <data> <key0> <key1> ... 0.
// entries are allocated in 256-word 'pages', controlled by word 0, which
// is a free pointer.

public class DHT_RDMA implements DHT {

}
