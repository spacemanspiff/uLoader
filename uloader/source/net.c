
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <ogcsys.h>
#include <network.h>
#include <ogc/lwp_watchdog.h>

#include <sys/types.h>
#include <sys/errno.h>
#include <fcntl.h>

#include <network.h>
#include "net.h"

char net_error[256];

static int netInit = 0;

void http_deinit(void)
{
	net_deinit();
	netInit = 0;
}

int http_init(void) 
{
	int retval;

	if (!netInit) {
		while (1) {
			retval = net_init ();
			if (retval < 0) {
				if (retval != -EAGAIN) {
					sprintf (net_error, "net_init failed: %d", retval);
					return -1;
				}
			}
			if (!retval) 
				break;

			usleep(100000);
		}
		sleep(1);
		netInit = 1;
	}
	return 1;

}
