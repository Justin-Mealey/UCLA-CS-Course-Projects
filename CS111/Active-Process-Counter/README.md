# A Kernel Seedling
This project inserts a module into the kernel that prints out the number of processes currently running.

## Building
```shell
make
sudo insmod proc_count.ko
```
In the directory with our 4 lab files, the make command will create many files that will allow our code to run, including a 
.ko object file that is linked to some of the attributes the kernel needs to run the module.

The sudo insmod command inserts our module into the kernel so that it can be ran. sudo allows us to have superuser privileges, and insmod does the actual insertion.

## Running
```shell
cat /proc/count
```
Example Result: 146

The following runs our module, and prints the result (the number of processes we counted and printed within the module). 
In this example, I had 146 processes currently running in the virtual machine the module was ran on.

## Cleaning Up
```shell
make clean
sudo rmmod proc_count.ko
```
The make clean command cleans up all of the files that were created by make. This cleans up the working directory, and can be done after the module was inserted into the kernel (and the module will still run).

The sudo rmmod command removes our inserted module from the kernel, and can no longer be ran. Therefore, /proc/count will no longer exist, and trying to run our module will give an error because our module no lomger exists in the kernel.

## Testing
```python
python -m unittest
```
Running the above command runs the Python test script, and outputs the results (if the module correctly outputted the number of processes or not).

Example result:
Ran 3 tests in 3.251s

OK

## Release Version
```shell
uname -r -s -v
```
The following command output Linux 5.14.8-arch1-1 #1 SMP PREEMPT Sun, 26 Sep 2021 19:36:15 +0000
This shows that I am using kernel release version 5.14.8