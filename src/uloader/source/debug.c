#include "debug.h"

#include "mload.h"
#include <gccore.h>

#include <network.h>

#include "net.h"

#define STACKSIZE		8192

#define LOG_MODE_SD       1
#define LOG_MODE_UDP      2

#define DEBUG_UDP_PORT	5000
#define DEBUG_UDP_IP 	"192.168.0.2"

char log_buffer[4096];


#ifdef ENABLE_DEBUG
volatile int debug_thread_running = 0;

static u8 debug_stack[STACKSIZE];
static int log_mode = LOG_MODE_SD;

static lwp_t h_debug;

extern int sd_ok;

#define MAX_BUFFER 4096

static char log_data[MAX_BUFFER];

static void *debug_thread(void *priv)
{
	while (debug_thread_running) {
		mload_init();
		int len = mload_get_log();
		if (len > 0)  {
			mload_read(log_data, len);
		}
		mload_close();
		usleep(1000*2);
	}
	return NULL;
}

int s_udp = -1;

/*
static int udp_close()
{
	if (s_udp != -1)
		close(s_udp);
	return 1;
}
*/

static int udp_init()
{
	network_init();
	struct sockaddr_in si_srv;
	
	net_init();

	if ((s_udp = net_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)

	memset((char *) &si_srv, 0, sizeof(si_srv));
	si_srv.sin_family = AF_INET;
	si_srv.sin_port = htons(DEBUG_UDP_PORT);
	if (inet_aton(DEBUG_UDP_IP, &si_srv.sin_addr)==0) {
		close(s_udp);
		s_udp = -1;
	}
	return s_udp;
}


static int debug_log_udp(char *log, int len)
{
	if (s_udp == -1)
		udp_init();

	if (net_write(s_udp, log, len) == -1)
		return 0;

	return 1;
}

static int debug_log_sd(char *log, int len)
{
	static int first = 1;

	if (sd_ok) {
		FILE *fp=fopen("sd:/uloader_log.txt",first?"w":"a");
		first = 0;
		if(fp) {
			fwrite(log, 1, len, fp);
			fclose(fp);
		} else
			return 0;
	} else
		return 0;
	return 1;
}

int debug_log(char *log)
{
	int res;
	int len = 0;

	while (len < MAX_BUFFER && log[len])
		len++;

	log[len] = 0;

	switch(log_mode) {
		case LOG_MODE_SD:
			res = debug_log_sd(log, len);
			break;

		case LOG_MODE_UDP:
		default:
			res = debug_log_udp(log, len);
			break;
	}
	return res;
}


int debug_stop()
{
	if (debug_thread_running) {
		debug_thread_running = 0;
		usleep(20 * 1000);
	}
	return 1;
}

int debug_init(int log_thread)
{
	log_mode = LOG_MODE_SD;

	if (log_thread) {
		debug_thread_running = 1;
		if (LWP_CreateThread(&h_debug, debug_thread, NULL, debug_stack, STACKSIZE, 80) == -1) {
			return 0;
		}
	} else
		debug_thread_running = 0;

	debug_log("\n-----------------------------------------------\nuLoader starting\n");
	return 1;
}

int debug_dump_mem(u8 *mem, int len, char  *description)
{
	char filename[256];
	snprintf(filename,256,"sd:/uloader_%s.bin",description);

	FILE *fp=fopen(filename,"w");
	if(fp) {
		fwrite(mem, 1, len, fp);
		fclose(fp);
		return 0;
	}
	return 1;
}

#endif
