#ifndef DUHDRAW_SAUCE_H
#define DUHDRAW_SAUCE_H

#include <stdio.h>

/*
 * FIXME: Check the SAUCE spec!
 */
struct sauce_record {
	char		magic[7];	/* SAUCE00 */
	char		workname[35];
	char		author[20];
	char		group[20];
	char		date[8];
};

struct sauce_info {
	char		*workname;
	char		*author;
	char		*group;
	char		*date;
};

struct sauce_info *sauce_info_read(FILE *input);
void sauce_info_delete(struct sauce_info *info);

#endif /* DUHDRAW_SAUCE_H */
