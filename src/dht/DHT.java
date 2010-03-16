package dht;

public interface DHT {
	String get(String key);
	void put(String key, String val);
	boolean has(String key);
}
