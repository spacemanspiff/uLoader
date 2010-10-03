#include "wiinnertag.h"
#include "http.h"

#include <stdio.h>

#define WIINNERTAG_KEY_SIZE 50
#define URL_SIZE 200

static const char *wiinnertag_url_base = "http://www.wiinnertag.com/wiinnertag_scripts/update_sign.php";
static const char *wiinnertag_key = "sd:/wiinnertag.key";

int updateWiinnerTag(const u8 *title_id, char *exit_msg)
{
	int i = 0;
	char url[URL_SIZE];
	char key[WIINNERTAG_KEY_SIZE+1];
	
	FILE *fp = fopen(wiinnertag_key, "r");

	if (!fp) {
		snprintf(exit_msg, 40, "No WinnerTag Key");
		return FALSE;
	}
		
	fgets(key, WIINNERTAG_KEY_SIZE, fp);
	fclose(fp);
	
	key[WIINNERTAG_KEY_SIZE] = 0;
	
	while (i < WIINNERTAG_KEY_SIZE && key[i]) {
		if (key[i] == '\n' || key[i] == '\r') {
			key[i] = 0;
		} else
			i++;
	}

	u8 *temp_buf = NULL;
	u32 temp_size = 0;

	snprintf(url, URL_SIZE, "%s?key=%s&game_id=%s", wiinnertag_url_base, key, title_id);

	int res = download_file(url, &temp_buf, &temp_size) ;
	if (res != 0)
		snprintf(exit_msg, 40, "WinnerTag server error %d", res);

	return res == 0;
}
