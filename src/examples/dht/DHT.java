package examples.dht;

public interface DHT {
	int get(String key);
	void put(String key, int val);
	boolean has(String key);
	int[] match(int key, byte[] pattern);

    // key-scanning iface
    int getNBins();
    int getHead(int bin);
    int getNext(int ptr);
    String getKey(int ptr);
}
