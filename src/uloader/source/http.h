#ifndef _HTTP_H_
#define _HTTP_H_

#include <gctypes.h>

typedef enum {
	HTTPR_OK,
	HTTPR_ERR_CONNECT,
	HTTPR_ERR_REQUEST,
	HTTPR_ERR_STATUS,
	HTTPR_ERR_TOOBIG,
	HTTPR_ERR_RECEIVE
} http_res;

bool http_request (const char *url, const u32 max_size);
bool http_get_result (u32 *http_status, u8 **content, u32 *length);
int download_file(char *url, u8 **outbuf, u32 *outlen);
void http_deinit(void);

#endif

