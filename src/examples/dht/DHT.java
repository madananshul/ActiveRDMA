package examples.dht;

public interface DHT {
	int get(String key);
	void put(String key, int val);
	boolean has(String key);

    // key-scanning iface
    int getNBins();
    int getHead(int bin);
    int getNext(int ptr);
    String getKey(int ptr);
}
