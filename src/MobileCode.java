import java.util.concurrent.atomic.AtomicInteger;

public class MobileCode
{
	public static int execute(AtomicInteger[] mem, int i) {
		mem[i].set(42);
		return mem[i].get();
	}
}
