package dfs;
import examples.dht.DHT;

public interface DFS {
    int lookup(String name); // filename -> inode (must exist)
    int create(String name); // filename -> inode (creates new)

    int getLen(int inode);
    void setLen(int inode, int len); // sets length of file
    int get(int inode, byte[] buffer, int off, int len); // gets range
    int put(int inode, byte[] buffer, int off, int len); // puts range

    // transparency into underlying DHT
    DHT dht();
}
