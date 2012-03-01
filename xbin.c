#include "xbin.h"

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

struct xbin_image *xbin_load_image(int fd)
{
	struct xbin_image *xbin;
	struct stat st;
	void *p;

	xbin = calloc(1, sizeof *xbin);
	if (!xbin)
		return NULL;

	if (fstat(fd, &st) < 0)
		goto error_free;

	p = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (p == MAP_FAILED)
		goto error_free;

	xbin->xb_header = p;

	if (memcmp(xbin->xb_header->ID, XBIN_MAGIC, sizeof(xbin->xb_header->ID)))
		goto error_munmap;

	p += sizeof(struct xbin_header);

	if (xbin->xb_header->Flags & XBIN_FLAG_PALETTE) {
		xbin->xb_palette = p;

		p += sizeof(struct xbin_palette);
	}

	if (xbin->xb_header->Flags & XBIN_FLAG_FONT) {
		xbin->xb_font = p;

		p += sizeof(struct xbin_font);
	}

	if (xbin->xb_header->Flags & XBIN_FLAG_COMPRESS) {
		assert(0);
		goto error_munmap;
	}

	if (xbin->xb_header->Flags & XBIN_FLAG_512CHARS) {
		assert(0);
		goto error_munmap;
	}

	xbin->xb_data = p;

	return xbin;

error_munmap:
	munmap(p, st.st_size);
error_free:
	free(xbin);

	return NULL;
}
