
#include "remote.h"

#define REMOTE_PRIORITY 79
#define STACKSIZE		1024*128

static lwpq_t remote_queue;
static lwp_t h_remote=-1;
static int remote_thread_status=0;
static int remote_loop=1;
static int remote_command=0;

static void *remote_arg[16];
static int remote_return=0;

static void * remote_thread(void *priv)
{
void (*func)(void);

	LWP_InitQueue(&remote_queue);

	while(remote_loop)
	{
	
	if(remote_command!=2) {remote_thread_status=0;LWP_ThreadSleep(remote_queue);}
	if(!remote_loop) break;
	remote_thread_status=1;

	switch(remote_command)
		{
		case 1:
			remote_return=USBStorage2_Init();
			remote_command=0;
			break;
		case 2:
			func=remote_arg[0];
			func();
			break;
		
		}

	}

	LWP_CloseQueue(remote_queue);

return NULL;
}



int remote_status()
{
	return remote_thread_status;
}

int remote_init(void)
{


	remote_command=0;
    remote_loop=1;

	if(LWP_CreateThread(&h_remote,(void *) remote_thread, NULL, NULL,STACKSIZE,REMOTE_PRIORITY)==-1)
			{
			remote_thread_status=0;
			return -1;
			}
	LWP_ThreadSignal(remote_queue);

return 0;
}

void remote_end()
{

	if(h_remote<0)  return;

	while(remote_status());


	remote_command=0;
	remote_loop=0;

	LWP_ThreadSignal(remote_queue);
	LWP_JoinThread(h_remote,NULL);

	h_remote=-1;
}

int remote_ret()
{
	if(remote_status()) return REMOTE_BUSY;
	return remote_return;
}

int remote_USBStorage2_Init(void)
{

	if(remote_status()) return REMOTE_BUSY;
	
	remote_thread_status=1;
	remote_return=-1;
	remote_command=1;
	LWP_ThreadSignal(remote_queue);

	return 0;
}



int remote_call(void *func)
{
if(remote_status()) return REMOTE_BUSY;
	
	remote_thread_status=1;
	remote_return=0;
	remote_arg[0]=func;
	remote_command=2;
	LWP_ThreadSignal(remote_queue);

	return 0;
}

void remote_call_abort()
{

	if(remote_command==2) remote_command=0;

}

