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

MessageType MesType;

int pipefd[11][11][2];

int send(void * self, local_id dst, const Message * msg)
{
  local_id self_id = *(local_id*)self;
  int size = sizeof(msg->s_header) + (msg->s_header).s_payload_len;
  
  if(write(pipefd[self_id][dst][1], (void*) msg, size) == -1)
    return -1;
  return 0;
}


int send_multicast(void * self, const Message * msg)
{
  int i;
  local_id self_id = *(local_id*)self;
  for(i = 0; i <= process_count; i++)
    {
      if(i != self_id)
	if(send(self, i, msg) == -1) 
	  return -1;
    }
      return 0;  
}


int receive(void * self, local_id from, Message * msg)
{
  local_id self_id = *(local_id*)self;
  int size = sizeof(msg->s_header);

  if(read(pipefd[from][self_id][0], (void*) msg, size) <= 0)
  return -1;
  if(read(pipefd[from][self_id][0], (void*) msg->s_payload, msg->s_header.s_payload_len) <= 0)
    return -1;
  return 0;
}


//int receive_any(void * self, Message * msg);


int main(int argc, char* argv[])
{
  char buff_string[4088];
  Message* message = malloc(sizeof(Message));
  Message* message_rec;
  pid_t c_pid;
  int i, j, index;
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
  for(i = 0; i < process_count  + 1; i++)
    {
      for(j = 0; j < process_count + 1; j++)
	{//maybe need not to open pipe when i == j
	  if(pipe(pipefd[i][j]) == -1)
	    {
	      perror("Error while creating pipe:");
	      exit(EXIT_FAILURE);
	    }
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
  for(i = 0; i < process_count + 1; i++)
    {
      if(i != curr_pid)
	{
	  close(pipefd[curr_pid][i][0]);
	  close(pipefd[i][curr_pid][1]);
	}
    }

  message_rec = malloc(sizeof(Message));

  if(curr_pid == 0)
    {
      for(i = 1; i < process_count; i++)
	{
	  if(i != curr_pid)
	    {
	      receive(&curr_pid, i, message_rec);
	    }
	}
      sprintf(buff_string, log_received_all_started_fmt, curr_pid);
      write(STDOUT_FILENO, buff_string, strlen(buff_string));//logging
      
      for(i = 0; i < process_count; i++)
	wait(NULL);

      exit(EXIT_SUCCESS);
    }
  else
    {
      sprintf(buff_string, log_started_fmt, curr_pid, getpid(), getppid());
      message->s_header.s_magic = MESSAGE_MAGIC;
      message->s_header.s_type = MesType;
      message->s_header.s_payload_len = strlen(buff_string);
      strcpy(message->s_payload, buff_string);
      write(STDOUT_FILENO, message->s_payload, strlen(message->s_payload));
      //logging
      if(send_multicast(&curr_pid, message) == -1)
	{
	  printf("Error while sending. Process_num %d\n", curr_pid);
	  exit(EXIT_FAILURE);
	}
      for(i = 1; i <= process_count; i++)
	{
	  if(i != curr_pid)
	    {
	      if(receive(&curr_pid, i, message_rec) == -1)
		{
		  printf("Error while receiving. Process_num %d\n", curr_pid);
		  exit(EXIT_FAILURE);
		}	
	      printf("received mes proc num %d :%s\n",curr_pid, message_rec->s_payload);
	    }
	}
      sprintf(buff_string, log_received_all_started_fmt, curr_pid);
      write(STDOUT_FILENO, buff_string, strlen(buff_string));//logging
    }
  exit(EXIT_SUCCESS);
}
