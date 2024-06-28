## UID: 005961729

## Pipe Up

The pipe program works very similarly to the Linux pipe, running the first program passed to it, using that programs output as input for the next program passed to it, etc. until finally returning the last program's output to stdout.

## Building

```make```
This command runs the default behavior of the Makefile, compiling the pipe.c file into an executable that can be ran.

## Running

```./pipe ls cat wc```
Creating a process of our newly built executable in the terminal, we pass three programs to our pipe process. We expect the output of the ls command to be fed to the cat command, the output of cat to be fed to wc, and the output of wc to bed written to stdout (printed to the terminal). Indeed, we see this result:
```7       7      63```
Which is the same output as ls | cat | wc, and displays the # of lines, words, and bytes in the current directory (seen by ls).

## Cleaning up

```make clean```
This command undoes the work make did, removing the binary executable and other files that were created by make, hence cleaning the working directory.
