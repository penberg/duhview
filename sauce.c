#include "sauce.h"

#include "msdos.h"

#include <string.h>
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

	info->workname = sauce_read(record->Title, sizeof record->Title);
	info->author = sauce_read(record->Author, sizeof record->Author);
	info->group = sauce_read(record->Group, sizeof record->Group);
	info->date = sauce_read(record->Date, sizeof record->Date);

	return info;
}

struct sauce_info *sauce_info_read(FILE *input)
{
	struct sauce_record record;

	fseek(input, -sizeof(record), SEEK_END);

	fread(&record, sizeof record, 1, input);

	if (memcmp(record.ID, "SAUCE", sizeof(record.ID)))
		return NULL;

	if (memcmp(record.Version, "00", sizeof(record.Version)))
		return NULL;

	return sauce_record_to_info(&record);
}
