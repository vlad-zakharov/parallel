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
#include "banking.h"

local_id curr_pid = 0;
int process_count = 1;
extern char* optarg;

MessageType MesType;

int pipefd[MAX_PROCESS_ID][MAX_PROCESS_ID][2];


int log_event(char* message, int fd)
{
  if(write(STDOUT_FILENO, message, strlen(message)) < 0) perror("Error while logging");//logging
  if(write(fd, message, strlen(message)) < 0) perror("Error while logging");
  return 0;
}

void transfer(void * parent_data, local_id src, local_id dst,
              balance_t amount)
{
  // student, please implement me
}


int init_pipes(int process_count)
{
  //file for logging openned pipes
  int i, j, pipelog = open("pipes.log", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);

  if(pipelog == -1)
    perror("Error while opening pipes.log ");

  //Creating pipes (two for each relation)
  for(i = 0; i < process_count  + 1; i++)
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
  //All pipes are openned - no need to keep "pipes.log" openned
  close(pipelog);
}

int main(int argc, char * argv[])
{
  char buff_string[MAX_MESSAGE_LEN];
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


  //Openning all pipes
  init_pipes(process_count);


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
      for(j = 0; j < process_count + 1; j++)
	{
	  if(i != j)
	    {
	      if((i != curr_pid) && (j != curr_pid))
		{
		  close(pipefd[i][j][0]);
		  close(pipefd[i][j][1]);
		}
	    }
	}
      if(i != curr_pid)
	{
	  close(pipefd[curr_pid][i][0]);
	  close(pipefd[i][curr_pid][1]);
	}
    }

  message_rec = malloc(sizeof(Message));



  //If it is process K
  if(curr_pid == 0)
    {
      //Receiving STARTED from all C-processes
      for(i = 1; i <= process_count; i++)
	{
	  if(i != curr_pid)
	    {
	      receive(&curr_pid, i, message_rec);
	      if(message_rec->s_header.s_type != STARTED) i--;
	    }
	}

      //Calling bank_robbery function
      //bank_robbery(parent_data);

      //
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
    //bank_robbery(parent_data);
    //print_history(all);

    return 0;
}
