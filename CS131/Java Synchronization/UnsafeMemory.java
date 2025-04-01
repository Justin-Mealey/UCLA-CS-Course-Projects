//follows format: time timeout 3600 java UnsafeMemory Platform 5 Synchronized 8 100000000
//                                                      0      1      2       3     4

import java.lang.management.ManagementFactory;

class UnsafeMemory {
    public static void main(String args[]) {
		if (args.length != 5)
			usage(null);
		try {
			boolean virtual;
			if (args[0].equals("Platform"))
			virtual = false;
			else if (args[0].equals("Virtual"))
			virtual = true;
			else
			throw new Exception(args[0]);

			var nValues = (int) argInt(args[1], 0, Integer.MAX_VALUE);

			State s;
			if (args[2].equals("Null"))
				s = new NullState(nValues);
			else if (args[2].equals("Synchronized"))
				s = new SynchronizedState(nValues);
			else if (args[2].equals("Unsynchronized"))
				s = new UnsynchronizedState(nValues);
			else if (args[2].equals("AcmeSafe"))
				s = new AcmeSafeState(nValues);
			else
			throw new Exception(args[2]);

			var nThreads = (int) argInt(args[3], 1, Integer.MAX_VALUE);
			var nTransitions = argInt(args[4], 0, Long.MAX_VALUE);

			dowork(virtual, nThreads, nTransitions, s);
			test(s.current());
			System.exit(0);
		} catch (Exception e) {
			usage(e);
		}
    }

    private static void usage(Exception e) {
		if (e != null)
			System.err.println(e);
		System.err.println("Arguments: [Platform|Virtual] nvalues model"
				+ " nthreads ntransitions\n");
		System.exit(1);
    }

    private static long argInt(String s, long min, long max) {
	var n = Long.parseLong(s);
	if (min <= n && n <= max)
	    return n;
	throw new NumberFormatException(s);
    }

    private static void dowork(boolean virtual, int nThreads, long nTransitions, State s)
    throws InterruptedException {
		var builder = virtual ? Thread.ofVirtual() : Thread.ofPlatform();
		var test = new SwapTest[nThreads];
		var t = new Thread[nThreads];

		for (var i = 0; i < nThreads; i++) {
			var threadTransitions =
			(nTransitions / nThreads
			+ (i < nTransitions % nThreads ? 1 : 0));
			test[i] = new SwapTest(threadTransitions, s);
			t[i] = builder.unstarted(test[i]);
		}
		var realtimeStart = System.nanoTime();
		for (var i = 0; i < nThreads; i++)
			t[i].start();
		for (var i = 0; i < nThreads; i++)
			t[i].join();
		var realtimeEnd = System.nanoTime();
		long realtime = realtimeEnd - realtimeStart;
		double dTransitions = nTransitions;

		System.out.format("Total real time %g s\n", realtime / 1e9);

	  	System.out.format("Average real swap time %g ns\n", realtime / dTransitions * nThreads);
    }

    private static void test(long[] output) {
	long osum = 0;
	for (var i = 0; i < output.length; i++)
	    osum += output[i];
	if (osum != 0)
	    error("output sum mismatch", osum, 0);
    }

    private static void error(String s, long i, long j) {
	System.err.format("%s (%d != %d)\n", s, i, j);
	System.exit(1);
    }
}
