#include "common.h"
#include "ipc.h"
#include "pa1.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <getopt.h>
#include <sys/wait.h>

local_id curr_pid = 0;
int process_count = 1;
extern char* optarg;

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


int log_event(char* message, int fd)
{
  if(write(STDOUT_FILENO, message, strlen(message)) < 0) perror("Error while logging");//logging
  if(write(fd, message, strlen(message)) < 0) perror("Error while logging");
  return 0;
}


//int receive_any(void * self, Message * msg);


int main(int argc, char* argv[])
{
  char buff_string[4088];
  Message* message = malloc(sizeof(Message));
  Message* message_rec;
  pid_t c_pid;
  int i, j;
  char opt = -1;
  int pipelog, eventslog;
  do
    {
      opt = getopt(argc, argv, "p:");
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
    }while(opt != -1);
    
  //Creating pipes (two for each relation)

  //file for logging openned pipes
  pipelog = open("pipes.log", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);

  if(pipelog == -1)
    perror("Error while opening pipes.log ");

  for(i = 1; i < process_count  + 1; i++)
    {
      for(j = 0; j < process_count + 1; j++)
	{//maybe need not to open pipe when i == j
	  if(i != j)
	    {
	      if(pipe(pipefd[i][j]) == -1)
		{
		  perror("Error while creating pipe:");
		  exit(EXIT_FAILURE);
		}
	      sprintf(buff_string, "Pipe from %d to %d is openned\n", i, j);
	      write(pipelog, buff_string, strlen(buff_string));
	    }
	}
    }
  close(pipelog);


  //Opening file for events log
  eventslog = open("events.log", O_WRONLY | O_CREAT | O_TRUNC,  S_IRWXU | S_IRWXG | S_IRWXO);

  if(eventslog == -1)
    perror("Error while opening events.log ");

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
      for(i = 1; i <= process_count; i++)
	{
	  if(i != curr_pid)
	    {
	      receive(&curr_pid, i, message_rec);
	      if(message_rec->s_header.s_type != STARTED) i--;
	    }
	}

      for(i = 1; i <= process_count; i++)
	{
	  if(i != curr_pid)
	    {
	      receive(&curr_pid, i, message_rec);
	      if(message_rec->s_header.s_type != DONE) i--;
	    }
	}


      for(i = 0; i < process_count; i++)
	wait(NULL);
      close(eventslog);

      exit(EXIT_SUCCESS);
    }
  else
    {
      
      sprintf(buff_string, log_started_fmt, curr_pid, getpid(), getppid());
      
      //logging
      log_event(buff_string, eventslog);

      MesType = STARTED;
      message->s_header.s_magic = MESSAGE_MAGIC;
      message->s_header.s_type = MesType;
      message->s_header.s_payload_len = strlen(buff_string) + 1;
      strcpy(message->s_payload, buff_string);
      if(send_multicast(&curr_pid, message) == -1)
	{
	  printf("Error while sending. Process_num %d\n", curr_pid);
	  exit(EXIT_FAILURE);
	}
      //
      for(i = 1; i <= process_count; i++)
	{
	  if(i != curr_pid)
	    {
	      if(receive(&curr_pid, i, message_rec) == -1)
		{
		  printf("Error while receiving. Process_num %d\n", curr_pid);
		  exit(EXIT_FAILURE);
		}	
	      if(message_rec->s_header.s_type != STARTED) i--;
	    }
	}
      sprintf(buff_string, log_received_all_started_fmt, curr_pid);

      //logging work
      log_event(buff_string, eventslog);
      
      MesType = DONE;

      sprintf(buff_string, log_done_fmt, curr_pid);
      //logging 
      log_event(buff_string, eventslog);

      message->s_header.s_magic = MESSAGE_MAGIC;
      message->s_header.s_type = MesType;
      message->s_header.s_payload_len = strlen(buff_string) + 1;
      strcpy(message->s_payload, buff_string);
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
	      if(message_rec->s_header.s_type != DONE) i--;
	    }
	}
      sprintf(buff_string, log_received_all_done_fmt, curr_pid);

      //logging receiveng all messages
      log_event(buff_string, eventslog);
      close(eventslog);
    }
  exit(EXIT_SUCCESS);
}
