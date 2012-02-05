#include "sauce.h"

#include "msdos.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static struct sauce_info *sauce_info_new(void)
{
	return calloc(1, sizeof(struct sauce_info));
}

void sauce_info_delete(struct sauce_info *info)
{
	free(info->workname);
	free(info->author);
	free(info->group);
	free(info->date);
	free(info);
}

static char *strstrip(char *s)
{
	size_t len;
	char *end;

	if (!s)
		return s;

	len = strlen(s);
	if (!len)
		return s;

	end = s + len - 1;

	while (end >= s && isspace(*end))
		end--;

	end[1] = '\0';

	return s;
}

static char *sauce_read(const char *s, size_t len)
{
	char *ret;

	ret = malloc(len + 1);
	if (!ret)
		return NULL; 

	strncpy(ret, s, len);

	ret[len] = '\0';

	return strstrip(ret);
}

static struct sauce_info *sauce_record_to_info(struct sauce_record *record)
{
	struct sauce_info *info;

	info = sauce_info_new();
	if (!info)
		return NULL;

	info->workname = sauce_read(record->workname, sizeof record->workname);
	info->author = sauce_read(record->author, sizeof record->author);
	info->group = sauce_read(record->group, sizeof record->group);
	info->date = sauce_read(record->date, sizeof record->date);

	return info;
}

struct sauce_info *sauce_info_read(FILE *input)
{
	struct sauce_record record;

	for (;;) {
		int ch;

		ch = fgetc(input);
		if (ch < 0)
			return NULL;

		if (ch < 0 || ch == MSDOS_EOF)
			break;
	}

	fgetc(input);	/* Extra MS-DOS EOF marker */

	memset(&record, 0, sizeof record);

	fread(&record, sizeof record, 1, input);

	return sauce_record_to_info(&record);
}
