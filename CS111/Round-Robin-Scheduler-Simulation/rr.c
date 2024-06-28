#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process
{
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  u32 time_left;
  bool ran_yet;
  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end)
{
  u32 current = 0;
  bool started = false;
  while (*data != data_end)
  {
    char c = **data;

    if (c < 0x30 || c > 0x39)
    {
      if (started)
      {
        return current;
      }
    }
    else
    {
      if (!started)
      {
        current = (c - 0x30);
        started = true;
      }
      else
      {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data)
{
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++]))
  {
    if (c < 0x30 || c > 0x39)
    {
      exit(EINVAL);
    }
    if (!started)
    {
      current = (c - 0x30);
      started = true;
    }
    else
    {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path,
                    struct process **process_data,
                    u32 *process_size)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1)
  {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
  {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL)
  {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i)
  {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }

  munmap((void *)data, size);
  close(fd);
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    return EINVAL;
  }
  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);

  struct process_list list;
  TAILQ_INIT(&list);

  u32 total_waiting_time = 0;
  u32 total_response_time = 0;

  /* Your code here */
  if(quantum_length < 1){
    perror("quantum");
    exit(EINVAL);
  }

  // Initialize 
  struct process* running_process = NULL;
  u32 current_time = 0;
  u32 processes_left = size;
  u32 turn_finished_time = 0; // time that the currently running process will be done its turn
  for (int i = 0; i < size; i++){ //initialize added fields
    data[i].ran_yet = false;
    data[i].time_left = data[i].burst_time;
  }
  
  while(processes_left > 0){ //each iteration covers one "second"
    for (int id = 0; id < size; id++){ 
      if (data[id].arrival_time == current_time){ //just arrived, put at end of queue
        TAILQ_INSERT_TAIL(&list, &data[id], pointers); 
      }
    }

    PICK_PROCESS_TO_RUN:
      if (running_process == NULL){ //try to schedule one
        if (!TAILQ_EMPTY(&list)){ //we do have an eligible process to run
          running_process = TAILQ_FIRST(&list);
          TAILQ_REMOVE(&list, running_process, pointers);

          // Handle response time
          if (running_process->ran_yet == false){
            total_response_time += current_time - running_process->arrival_time;
            running_process->ran_yet = true;
          }
          
          if (running_process->time_left < quantum_length) { //turn will finish before quantum is up
            turn_finished_time = current_time + running_process->time_left; 
          }
          else {
            turn_finished_time = current_time + quantum_length; 
          }
        }
      }
    
    if (running_process != NULL) //picked one, now run it
    {
      if (current_time == turn_finished_time) //we picked something that's finished its turn, handle turn switch
      {
        if (running_process->time_left == 0) //job completely finished
        {
          // Handle waiting time
          total_waiting_time += current_time - running_process->arrival_time - running_process->burst_time;
          processes_left--;
        }
        else //put at the back of the queue, round robin style
        {
          TAILQ_INSERT_TAIL(&list, running_process, pointers);
        }
        
        running_process = NULL;
        goto PICK_PROCESS_TO_RUN; //pick something else to run this "second" (if there is an eligible job)
      }
    }
    // Handle end of "second"
    current_time++;
    if (running_process != NULL){
      running_process->time_left --; 
    }
  }
  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
