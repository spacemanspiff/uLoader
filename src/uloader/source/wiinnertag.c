#include "wiinnertag.h"
#include "http.h"

//WiinnerTag
//
//
#include <stdio.h>

#define WIINNERTAG_KEY_SIZE 50
#define URL_SIZE 200

static const char *wiinnertag_url_base = "http://www.wiinnertag.com/wiinnertag_scripts/update_sign.php";
static const char *wiinnertag_key = "sd:/apps/uloader/wiinnertag.key";

int updateWiinnerTag(const u8 *title_id)
{
	char url[URL_SIZE];
	char key[WIINNERTAG_KEY_SIZE];
	
	FILE *fp;

	fp = fopen(wiinnertag_key, "r");

	if (!fp) 
		return FALSE;
		
	fgets(key, sizeof(key) ,fp);
	fclose(fp);

	u8 *temp_buf = NULL;
	u32 temp_size = 0;

	snprintf(url, URL_SIZE, "%s?key=%s&game_id=%s", wiinnertag_url_base, key, title_id);

	return download_file(url, &temp_buf, &temp_size) == 0;
}
