#ifndef DUHDRAW_SAUCE_H
#define DUHDRAW_SAUCE_H

#include <stdint.h>
#include <stdio.h>

struct sauce_record {
	char			ID[5];
	char			Version[2];
	char			Title[35];
	char			Author[20];
	char			Group[20];
	char			Date[8];
	uint32_t		FileSize;
	unsigned char		DataType;
	unsigned char		FileType;
	uint16_t		TInfo1;
	uint16_t		TInfo2;
	uint16_t		TInfo3;
	uint16_t		TInfo4;
	uint16_t		Flags;
	char			Filler[22];
} __attribute__ ((__packed__));

struct sauce_info {
	char		*workname;
	char		*author;
	char		*group;
	char		*date;
};

struct sauce_info *sauce_info_read(FILE *input);
void sauce_info_delete(struct sauce_info *info);

#endif /* DUHDRAW_SAUCE_H */
