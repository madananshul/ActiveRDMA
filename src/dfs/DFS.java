package dfs;

public interface DFS {
    int lookup(String name); // filename -> inode (must exist)
    int create(String name); // filename -> inode (creates new)

    int getLen(int inode);
    void setLen(int inode, int len); // sets length of file
    int get(int inode, int[] buffer, int off, int len); // gets range
    int put(int inode, int[] buffer, int off, int len); // puts range
}
