#include "common.h"
#include "ipc.h"
#include "pa1.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdout.h>
#include <time.h>

typedef struct
{
  int pipe_in[2];
  int pipe_out[2];
} bi_channel_t;

int get_ch_count(int process_count)
{
  return (1 + process_count) * process_count;
}

int get_ch_index(int pid_a, int pid_b, int process_count)
{
  if(pid_a < pid_b)
    {
      return ((2 * process_count - pid_a + 1) * pid_a) / 2 + pid_b - pid_a - 1;
    }
  else
    {
      return ((2 * process_count - pid_b + 1) * pid_b) / 2 + pid_a - pid_b - 1;
    }
}

int main(int argc, char* argv[])
{
  bi_channel_t pipefd_m[55];
  pid_t c_pid;
  int i, process_count = 1;
  char opt = -1;
  while((opt = getopt(argc, argv, "p:")) != -1)
    switch(opt)
      {
      case 'p':
	process_count = atoi(optarg);
	if((process_count < 1) || (process_count > 10))
	  {
	    printf("Bad process count. Must be between 1 and 10. Using default value (1).\n");
	    process_count = 1;
	  }
      }

  
  //Creating pipes (two for each relation)
  for(i = 0; i < get_ch_count(process_count); i++)
    {
      if(pipe(pipefd[i].pipe_in) == -1)
	{
	  perror("Error while creating pipe:");
	  exit(EXIT_FAILURE);
	}
      if(pipe(pipefd[i].pipe_out) == -1)
	{
	  perror("Error while creating pipe:");
	  exit(EXIT_FAILURE);
	}
    }

  //Forking new processes
  for(i = 0; i < process_count; i++)
    {
      c_pid = fork();
    }
}
