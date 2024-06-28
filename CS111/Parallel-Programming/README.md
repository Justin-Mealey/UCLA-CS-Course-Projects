# Hash Hash Hash

## Justin Mealey, UID:005961729
This lab involves programming two multi-threaded locking solutions for a hash table.

## Building
```make && make clean```
The make command runs the default behavior of the Makefile, compiling the .c and .h files into object files and an executable to run and test our implementations. The make clean command undoes the work make did, removing the executable and object files, cleaning the directory.

## Running
```
./hash-table-tester -t 8 -s 50000
Generation: 72,118 usec
Hash table base: 1,182,732 usec
  - 0 missing
```
To run, we call our tester executable with two arguments. -t determines the number of threads our program will use (4 by default), and -s determines the amount of hash table entries added per thread (25,000 by default).


## First Implementation
In the `hash_table_v1_add_entry` function, I locked a pthread_mutex_t lock at line 1, and released the lock at the end of the function (checking its return code at both steps). By making the entire function body a critical section, this solution is correct yet inefficient. We are guaranteed correctness as all hash table interactions are protected by the lock, so only one thread will be working with the hash table at any given time. However, this solution has somewhat poor performance due to maximizing the critical section's size.

Additionally, I created my lock variable globally in the file as a static variable. In the hash table create function, I initialize this lock. In the hash table destroy function, I destroy this lock. I also slightly refactored the add entry function into an if else (instead of an if statement that returns), so we can reach the same unlock code in both paths (instead of needing it twice). 

### Performance
```
./hash-table-tester -t 8 -s 50000
Generation: 73,688 usec
Hash table base: 1,176,471 usec
  - 0 missing
Hash table v1: 1,731,510 usec
  - 0 missing
```
Version 1 is slower than the base version. This is primarily due to thread overhead. The cost of creating, scheduling, and destroying multiple threads is significant, and makes Version 1 slow down considerably. We also observe a decrease in performance whenever a thread must wait to obtain a lock, and there will be more overhead in this case (waiting, blocking, scheduling new thread, etc.), further decreasing performance. 

## Second Implementation
In the `hash_table_v2_add_entry` function, I used one lock per hash table entry, for a total of HASH_TABLE_CAPACITY locks. Lock 1 protects the first entry (which is the first bucket of the hash table), lock 2 protects the second entry, etc. By locking every bucket, we are guaranteeing correctness as two threads interacting with different entries have no synchronization issues between the two of them. For example, if thread A accesses entry 3, that means it will be looking at the list or "bucket" number 3. If another thread, say thread B, wants to also access entry 3, it cannot as there would be two threads in a critical section (two threads looking at/modifying the list at the same time). This is prevented, as thread A aquired lock 3, so thread B would have to wait. However, if thread B wants to access entry 47, it should be able to as thread B modifying bucket 47 at the same time as thread A modifying bucket 3 will cause no problems (different lists). With this implementation, this would be possible, and thread B would simply aquire lock 47 and go ahead with its work. 

A couple of things needed to happen to get this to work. Firstly, struct hash_table_entry now includes a lock, so there is one lock per entry. Next, these locks must all be initialized in the create function, initializing one lock each iteration of the loop. The destroy function similarly destroys the locks one at a time as entries are freed. Inside the add_entry function, we get the entry and immediately try to obtain that entry's lock. Once acquired, we proceed as normal, releasing the lock before returning.
### Performance
```
./hash-table-tester -t 4 -s 100000
Generation: 80,050 usec
Hash table base: 1,423,119 usec
  - 0 missing
Hash table v1: 2,343,225 usec
  - 0 missing
Hash table v2: 430,075 usec
  - 0 missing
```

In this run, V2's speedup time is approximately 3.3x, meeting the threshold of base/(4 cores-1). This was ran with 4 cores, so improving by more than 3x is great performance. We accomplish this speedup by our bucket-locking approach. By taking a finer granularity in our locking (compared to locking the entire hash table in V1), we allow the threads to work together much better while still providing a correct implementation. With version 2, each thread can be doing work on different buckets at different times, achieving parallelism. Say we have 4 threads and 10,000 buckets. The odds that 2 of the 4 threads will need to work on the same bucket at the same time is low, so threads will wait for a small fraction of the run. Most of the time, all 4 threads will be doing seperate jobs on seperate buckets, and our computer can effectively do four things at once, significantly improving performance. 

## Cleaning up
```make clean```
The make clean command cleans our directory to the state it was in before the make command.
