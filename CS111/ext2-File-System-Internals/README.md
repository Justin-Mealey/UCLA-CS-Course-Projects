# Hey! I'm Filing Here
# Justin Mealey, 005961729

In this lab, I successfully implemented a 1 MiB mock Linux ext2 file system containing 2 directories, 1 regular file and 1 symbolic link.

## Building

```make```
The make command is used to create an object and executable file out of our ext2-create.c file. This allows us to run our program.
```./ext2-create```
The above command runs the executable we just created with make. This program creates a cs111-base.img file, which will be the file system we can mount, look at, and further test the contents of.

## Running

Some tests we can do to look inside our file system are:
```dumpe2fs cs111-base.img```
This program dumps the metadata of the file system I created, which is very helpful for looking at what is actually being created and debugging. 
```fsck.ext2 cs111-base.img```
This program does a thorough testing of the file system in 5 passes.


If we wish to mount the file system:
```mkdir mnt```
This makes a directory we can "plug" our file system onto.
```sudo mount -o loop cs111-base.img mnt```
Mount our file system onto the mnt directory.
```ls -ain mnt/```
Run the ls command on our file system, getting information about the files contained in what we just mounted.

For additional testing we can run:
```python3 -m unittest```



## Cleaning up

```make clean```
The make clean removes everything created by make, as well as the cs111-base.img file.

If we mounted the system, to clean it up we can run:
```sudo umount mnt```
Unmount our file system.
```rmdir mnt```
Remove the directory we mounted it on.