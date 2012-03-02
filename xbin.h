#ifndef DUHDRAW_XBIN_H
#define DUHDRAW_XBIN_H

#include <stdint.h>

#define XBIN_MAGIC		"XBIN"

enum {
	XBIN_FLAG_PALETTE		= (1U << 0),
	XBIN_FLAG_FONT			= (1U << 1),
	XBIN_FLAG_COMPRESS		= (1U << 2),
	XBIN_FLAG_NONBLINK		= (1U << 3),
	XBIN_FLAG_512CHARS		= (1U << 4),
};

struct xbin_header {
	uint8_t				ID[4];
	uint8_t				EofChar;
	uint16_t			Width;
	uint16_t			Height;
	uint8_t				Fontsize;
	uint8_t				Flags;
} __attribute__ ((packed));

struct xbin_rgb {
	uint8_t				r;
	uint8_t				g;
	uint8_t				b;
};

struct xbin_palette {
	struct xbin_rgb			Palette[16];
};

struct xbin_font {
	uint8_t				Font[4096];
};

struct xbin_image {
	struct xbin_header		*xb_header;
	struct xbin_palette		*xb_palette;
	struct xbin_font		*xb_font;
	char				*xb_data;
};

enum {
	XBIN_COMP_NONE			= 0x00,	/* No compression */
	XBIN_COMP_CHAR			= 0x40,	/* Character compression */
	XBIN_COMP_ATTR			= 0x80,	/* Attribute compression */
	XBIN_COMP_BOTH			= 0xc0, /* Character/attribute compression */
};

struct xbin_image *xbin_load_image(int fd);

#define XBIN_R(r) ((uint32_t) r * 4)
#define XBIN_G(g) (((uint32_t) g * 4) << 8)
#define XBIN_B(b) (((uint32_t) b * 4) << 16)

static inline uint32_t xbin_palette_rgba(struct xbin_palette *palette, uint8_t color)
{
	struct xbin_rgb *rgb = &palette->Palette[color];

	if (color == 0)
		return 0x00000000UL;
	else if (color == 1)
		return 0x00ffffffUL;

	return XBIN_R(rgb->r) | XBIN_G(rgb->g) | XBIN_B(rgb->b);
}

#endif /* DUHDRAW_XBIN_H */
