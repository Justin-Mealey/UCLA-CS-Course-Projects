import java.util.concurrent.ThreadLocalRandom;

class SwapTest implements Runnable {
    private long nTransitions;
    private State state;

    SwapTest(long n, State s) {
		nTransitions = n;
		state = s;
    }

    public void run() {
		var n = state.size();
		if (n <= 1)
			return;
		var rng = ThreadLocalRandom.current();
		var id = Thread.currentThread().threadId();

		for (var i = nTransitions; 0 < i; i--)
			state.swap(rng.nextInt(0, n), rng.nextInt(0, n));
    }
}
