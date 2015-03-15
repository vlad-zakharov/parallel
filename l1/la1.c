#include "common.h"
#include "ipc.h"
#include "pa1.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

local_id curr_pid = 0;
int process_count = 1;


typedef struct
{
  int pipe_in[2];
  int pipe_out[2];
} bi_channel_t;

bi_channel_t pipefd[55];

int get_ch_count(int process_count)
{
  return (1 + process_count) * process_count;
}


//pid_a shouldn't equals pid_b
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

int send(void * self, local_id dst, const Message * msg)
{
  local_id self_id = *(local_id*)self;
  int outfd;
  int size = sizeof(msg->s_header) + (msg->s_header).s_payload_len;
  int index = get_ch_index(self_id, dst, process_count);

  if(self_id < dst)
    outfd = pipefd[index].pipe_out[1];
  else 
    outfd = pipefd[index].pipe_in[1]; 
  if(write(outfd, (void*) msg, size) == -1)
    return -1;
  return 0;
}


int send_multicast(void * self, const Message * msg)
{
  int i;
  local_id self_id = *(local_id*)self;
  for(i = 0; i < process_count; i++)
    {
      if(i != self_id)
	if(send((void*)&self_id, i, msg) == -1) 
	  return -1;
    }
      return 0;  
}


int receive(void * self, local_id from, Message * msg)
{
  local_id self_id = *(local_id*)self;
  int infd;
  int size = sizeof(msg->s_header);
  int index = get_ch_index(self_id, from, process_count);

  if(self_id < dst)
    infd = pipefd[index].pipe_in[0];
  else 
    infd = pipefd[index].pipe_out[0]; 
  if(read(infd, (void*) msg, size) <0)
    return -1;
  if(read(infd, (void*) msg->s_payload, msg->s_header.s_payload_len) <0)
    return -1;
  return 0;
}


//int receive_any(void * self, Message * msg);


int main(int argc, char* argv[])
{
  char* buff_string;
  Message* message;
  pid_t c_pid;
  int i, index;
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
      if(c_pid == 0)
	{
	  curr_pid = i + 1;
	  break;
	}
    }

  //Deleting unusing pipes
  for(i = 0; i < process_count; i++)
    {
      if(i != curr_pid)
	{
	  index = get_ch_index(curr_pid, i, process_count);
	  if(curr_pid < i) 
	    {
	      close(pipefd[index].pipe_in[1]);	    
	      close(pipefd[index].pipe_out[0]);
	    }
	  else
	    {
	      close(pipefd[index].pipe_in[0]);
	      close(pipefd[index].pipe_out[1]);	    
	    }
	}
    }


  if(curr_pid == 0)
    {
      wait(NULL);
      exit(EXIT_SUCCESS);
    }
  else
    {
      sprintf(buff_string, log_started_fmt, curr_pid, getpid(), getppid());
      message->s_header.s_magic = MESSAGE_MAGIC;
      message->s_header.s_type = MessageType.STARTED;
      message->s_header.s_payload_len = strlen(buff_string);
      message_>s_payload = buff_string;
      send_multicast(&curr_pid, message);
      printf("pid %d\n", curr_pid);
    }
  exit(EXIT_SUCCESS);
}
