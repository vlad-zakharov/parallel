/*All pipes for that library should be created with flag O_NONBLOCK*/

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
