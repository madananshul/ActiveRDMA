package dht;

public interface DHT {
	int get(String key);
	void put(String key, int val);
	boolean has(String key);
}
