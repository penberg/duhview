#include "msdos.h"
#include "sauce.h"

#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

static void usage(char *program)
{
	printf("usage: %s <file>\n", basename(program));
}

int main(int argc, char *argv[])
{
	struct sauce_info *sauce;
	FILE *input;

	if (argc != 2) {
		usage(argv[0]);
		exit(1);
	}

	input = fopen(argv[1], "r");
	if (!input) {
		printf("Failed to open '%s': %s\n", argv[1], strerror(errno));
		exit(1);
	}	

	sauce = sauce_info_read(input);
	if (!sauce) {
		printf("No SAUCE information.\n");
		goto exit;
	}

	printf("Workname: '%s'\n", sauce->workname);
	printf("Author  : '%s'\n", sauce->author);
	printf("Group   : '%s'\n", sauce->group);
	printf("Date    : '%s'\n", sauce->date);

	sauce_info_delete(sauce);
exit:
	fclose(input);
	return 0;
}
