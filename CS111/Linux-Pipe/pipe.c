#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

int main(int argc, char *argv[])
{
  if (argc == 1){ //no args passed
    exit(EINVAL);
  }

  //Initialize pipes, buffers, and buffer size counters
  char buffers[argc - 1][4096];
  ssize_t numBytesFilled[argc - 1];
  for (int i = 0; i < argc - 1; i++) {
    numBytesFilled[i] = 0;
  }
  int pipes[(2*argc)-3][2];
  //Initialize pipes, buffers, and buffer size counters

  for (int arg = 1; arg < argc; arg++){
    if (arg == 1){ //special case, stdin is not changed
      if (pipe(pipes[0]) == -1) {
       perror("pipe creation failure\n");
       exit(1);
      }

      int rc = fork();
      if (rc == 0){ //child
        close(pipes[0][0]);
        dup2(pipes[0][1], STDOUT_FILENO); //stdout to pipe[0]
        close(pipes[0][1]);
        execlp(argv[arg], argv[arg], NULL);
        exit(errno); //only reached if execlp failed to turn process into argv[arg] program
      }
      else if (rc > 0){ //parent
        int pid = rc;
        int status = 0;
        waitpid(pid, &status, 0);

        close(pipes[0][1]);
        numBytesFilled[0] = read(pipes[0][0], buffers[0], 4096); //pipe[0] to buffer[0]
        close(pipes[0][0]);
      }
      else {
        fprintf(stderr, "failed fork()\n");
      }
    }
    else { //comments are for arg == 2 iteration
      if (pipe(pipes[arg - 1]) == -1) { //ex: this pipe connects argv[1] to argv[2] (1st arg to 2nd arg)
       perror("pipe creation failure\n");
       exit(1);
      }
      if (pipe(pipes[arg]) == -1) { //ex: this pipe connects argv[1] to buffer[1]  (2nd arg to 2nd buffer)
       perror("pipe creation failure\n");
       exit(1);
      }

      write(pipes[arg - 1][1], buffers[arg - 2], numBytesFilled[arg - 2]); //pipe[1] gets buffer[0] content (output of 1st cmd)
      close(pipes[arg - 1][1]);
      int rc = fork();
      if (rc == 0){ //child
        dup2(pipes[arg - 1][0], STDIN_FILENO); //take stdin from pipe[1]
        close(pipes[arg - 1][0]);
        dup2(pipes[arg][1], STDOUT_FILENO); //write stdout to pipe[2]
        close(pipes[arg][1]);
        execlp(argv[arg], argv[arg], NULL);
        exit(errno); //only reached if execlp failed to turn process into argv[arg] program
      }
      else if (rc > 0){ //parent
        int pid = rc;
        int status = 0;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && (WEXITSTATUS(status) != 0)){
          exit(WEXITSTATUS(status));
        }

        close(pipes[arg - 1][1]);
        close(pipes[arg][1]);
        numBytesFilled[arg - 1] = read(pipes[arg][0], buffers[arg - 1], 4096); //fill buffer[1] with pipe[2]
        close(pipes[arg - 1][0]);
        close(pipes[arg][0]);
      }
      else {
        fprintf(stderr, "failed fork()\n");
      }
    }

    if (arg == argc - 1){ //done last command, last buffer contains final output
      write(STDOUT_FILENO, buffers[arg - 1], numBytesFilled[arg - 1]);
    }
  }
}
