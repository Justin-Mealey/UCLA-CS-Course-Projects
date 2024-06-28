# You Spin Me Round Robin

The rr (Round Robin) program simulates a round robin scheduler of an Operating System. The user inputs a .txt file containing information about the processes being scheduled (id, arrival time, burst time) and a time slice number, and the program outputs the average waiting and response times of running each job to completion.

## Building

```make```

This command runs the default behavior of the Makefile, compiling the rr.c file into an executable rr that can be run from the terminal.

## Running

```./rr processes.txt 3```

The following command runs our round robin executable with the processes.txt file and a time slice of 3. This will run our simulation, scheduling and "running" the first process to arrive in the txt file, and so on until all processes finish "running". During the run, the program keeps track of the waiting and response times, outputting the following result:

```
Average waiting time: 7.00
Average response time: 2.75
```

Running ```./rr processes.txt 1``` we see the following result:

```
Average waiting time: 5.50
Average response time: 0.75
```

A smaller time slice results in a smaller average response time, as expected.

We can also run the python test script with the following command:
```python3 -m unittest```

## Cleaning up

```make clean```

This command undoes the work make did, removing the binary executable and other files that were created by make, hence cleaning the working directory. Running the test script creates a __pycache__ folder that would need to be manually deleted.
