#include "msdos.h"
#include "sauce.h"
#include "xbin.h"
#include "cp437-font.h"

#include <SDL/SDL.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdio.h>

#define ESC			27

struct cursor_pos {
	int			col;
	int			row;
};

struct char_attr {
	int			intensity;
	int			fg_color;
	int			bg_color;
};

enum {
	NORMAL,
	BRIGHT,
};

enum {
	BLACK			= 0,
	RED			= 1,
	GREEN			= 2,
	YELLOW			= 3,
	BLUE			= 4,
	MAGENTA			= 5,
	CYAN			= 6,
	WHITE			= 7,
};

struct bitmap_char {
	uint8_t			scanlines[16];
};

struct bitmap_font {
	struct bitmap_char	chars[256];
};

static void die(const char *s)
{
	fprintf(stderr, "Error: %s\n", s);
	exit(1);
}

#define CHAR_WIDTH		8
#define CHAR_HEIGHT		16

/*
 * Emulated text mode dimensions
 */

#define SCREEN_COLS		80
#define SCREEN_ROWS		43

#define BUFFER_COLS		80
#define BUFFER_ROWS		1000

/*
 * Visible window dimensions
 */

#define SCREEN_WIDTH		(SCREEN_COLS * CHAR_WIDTH)
#define SCREEN_HEIGHT		(SCREEN_ROWS * CHAR_HEIGHT)
#define SCREEN_DEPTH		32

#define BUFFER_WIDTH		(BUFFER_COLS * CHAR_WIDTH)
#define BUFFER_HEIGHT		(BUFFER_ROWS * CHAR_HEIGHT)
#define BUFFER_DEPTH		32

static char			mem[BUFFER_WIDTH * BUFFER_HEIGHT * BUFFER_DEPTH / 8];

#define R(x) (((uint32_t) x))
#define G(x) (((uint32_t) x) << 8)
#define B(x) (((uint32_t) x) << 16)


static uint32_t attr_color_to_rgba(int intensity, int color)
{
	static uint32_t normal2rgba[] = {
		[BLACK]		= R(0)   | G(0)   | B(0),
		[RED]		= R(170) | G(0)   | B(0),
		[GREEN]		= R(0)   | G(170) | B(0),
		[YELLOW]	= R(170) | G(85)  | B(0),
		[BLUE]		= R(0)   | G(0)   | B(170),
		[MAGENTA]	= R(170) | G(0)   | B(170),
		[CYAN]		= R(0)   | G(170) | B(170),
		[WHITE]		= R(170) | G(170) | B(170),
	};
	static uint32_t bright2rgba[] = {
		[BLACK]		= R(85)  | G(85)  | B(85),
		[RED]		= R(255) | G(85)  | B(85),
		[GREEN]		= R(85)  | G(255) | B(85),
		[YELLOW]	= R(255) | G(255) | B(85),
		[BLUE]		= R(85)  | G(85)  | B(255),
		[MAGENTA]	= R(255) | G(85)  | B(255),
		[CYAN]		= R(85)  | G(255) | B(255),
		[WHITE]		= R(255) | G(255) | B(255),
	};

	if (intensity == BRIGHT)
		return bright2rgba[color];

	return normal2rgba[color];
}

static void
draw_char(struct bitmap_font *font, int col, int row, uint32_t fg_rgba, uint32_t bg_rgba, uint8_t ch)
{
	struct bitmap_char *bmap;
	int start_x;
	int start_y;

	if (col < 0 || col >= BUFFER_COLS || row < 0 || row >= BUFFER_ROWS)
		return;

	bmap = &font->chars[ch];

	start_x = col * CHAR_WIDTH;
	start_y = row * CHAR_HEIGHT;

	for (int y = 0; y < CHAR_HEIGHT; y++) {
		for (int x = 0; x < CHAR_WIDTH; x++) {
			uint32_t rgba;
			uint32_t *p;
			int offset;

			offset = ((BUFFER_WIDTH * (start_y + y)) + (start_x + x)) * BUFFER_DEPTH/8;

			p = (void *) mem + offset;

			if (bmap->scanlines[y] & (1 << (CHAR_WIDTH - x - 1))) {
				rgba = fg_rgba;
			} else {
				rgba = bg_rgba;
			}

			*p = rgba;
		}
	}
}

/* Same as read(2) except that this function never returns EAGAIN or EINTR. */
static ssize_t xread(int fd, void *buf, size_t count)
{
        ssize_t nr;

restart:
        nr = read(fd, buf, count);
        if ((nr < 0) && ((errno == EAGAIN) || (errno == EINTR)))
                goto restart;

        return nr;
}

static ssize_t read_in_full(int fd, void *buf, size_t count)
{
        ssize_t total = 0;
        char *p = buf;

        while (count > 0) {
                ssize_t nr;

                nr = xread(fd, p, count);
                if (nr <= 0) {
                        if (total > 0)
                                return total;

                        return -1;
                }

                count -= nr;
                total += nr;
                p += nr;
        }

        return total;
}

static struct bitmap_font *load_font(const char *filename)
{
	struct bitmap_font *font;
	int fd;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return NULL;

	font = malloc(sizeof *font);
	if (!font)
		return NULL;

	if (read_in_full(fd, font, sizeof *font) != sizeof(*font))
		return NULL;

	return font;
}

#define MAX_ARGS	10

static int csi_sequence(FILE *input, struct cursor_pos *pos, struct char_attr *attr)
{
	int args[MAX_ARGS];
	int ndx = 0;

	memset(args, 0, sizeof(args));

	for (;;) {
		int ch;

		ch = fgetc(input);
		if (ch < 0)
			return -1;

		switch (ch) {
		case '0' ... '9':
			args[ndx] = args[ndx] * 10 + (ch - '0');
			break;
		case ';':
			if (ndx < MAX_ARGS)
				ndx++;
			break;
		case 'A': {
			int n = args[0];

			if (!n)
				n = 1;
			pos->row -= n;
			if (pos->row < 0)
				pos->row = 0;
			goto exit;
		}
		case 'B': {
			int n = args[0];

			if (!n)
				n = 1;
			pos->col += n;
			if (pos->col > BUFFER_ROWS-1)
				pos->col = BUFFER_ROWS-1;
			goto exit;
		}
		case 'C': {
			int n = args[0];

			if (!n)
				n = 1;
			pos->col += n;
			if (pos->col > BUFFER_COLS-1)
				pos->col = BUFFER_COLS-1;
			goto exit;
		}
#if 0
		case 'D': {
			int n = args[0];

			if (!n)
				n = 1;
			pos->col -= n;
			if (pos->col < 0)
				pos->col = 0;
			goto exit;
		}
#endif
		case 'H': {
			int n = args[0];
			int m = args[1];

			pos->col = n-1;
			pos->row = m-1;
			goto exit;
		}
		case 'J': {
#if 0
			int n = args[0];

#endif
			// TODO
			goto exit;
		}
		case 'm': {
			int i;

			for (i = 0; i <= ndx; i++) {
				int n = args[i];

				switch (n) {
				case 0:
					attr->intensity	= NORMAL;
					attr->fg_color	= WHITE;
					attr->bg_color	= BLACK;
					break;
				case 1:
					attr->intensity	= BRIGHT;
					break;
				case 5:
					/* blink on */
					break;
				case 25:
					/* blink off */
					break;
				case 30 ... 37:
					attr->fg_color = n - 30;
					break;
				case 40 ... 47:
					attr->bg_color = n - 40;
					break;
				default:
					break;
				}
			}
			goto exit;
		}

		/* Set screen mode */
		case 'h':
			fprintf(stderr, "ansi: screen mode %d not supported\n", args[0]);
			goto exit;

		/* Reset screen mode */
		case 'I':
			goto exit;

		default:
			fprintf(stderr, "ansi: unhandled command '%c' (0x%x) in escape sequence\n", ch, ch);
			goto exit;
		}
	}
exit:
	return 0;
}

static void
write_char(struct bitmap_font *font, struct cursor_pos *pos, struct char_attr *attr, int ch)
{
	switch (ch) {
	case '\b':
		if (pos->col > 0)
			pos->col--;
		break;
	case '\r':
		pos->col = 0;
		break;
	case '\n':
		pos->row++;
		break;
	case '\t':
		do {
			write_char(font, pos, attr, ' ');
		} while (pos->col % 8);
		break;
	default: {
		uint32_t fg_rgba, bg_rgba;

		fg_rgba = attr_color_to_rgba(attr->intensity, attr->fg_color);
		bg_rgba = attr_color_to_rgba(NORMAL, attr->bg_color);

		draw_char(font, pos->col++, pos->row, fg_rgba, bg_rgba, ch);
		break;
	}
	}
	if (pos->col == BUFFER_COLS) {
		pos->col = 0;
		pos->row++;
	}
}

static int load_img(FILE *input, struct bitmap_font *font)
{
	struct cursor_pos pos;
	struct char_attr attr;

	attr.intensity	= NORMAL;
	attr.fg_color	= WHITE;
	attr.bg_color	= BLACK;

	pos.col		= 0;
	pos.row		= 0;

	for (;;) {
		int ch;

		ch = fgetc(input);
		if (ch < 0 || ch == MSDOS_EOF)
			goto exit;

		if (ch == MSDOS_EOF)
			goto exit;

		if (ch != ESC) {
			write_char(font, &pos, &attr, ch);
			continue;
		}

		ch = fgetc(input);
		if (ch < 0 || ch == MSDOS_EOF)
			goto exit;

		if (ch != '[')
			continue;

		if (csi_sequence(input, &pos, &attr) < 0)
			goto exit;
	}
exit:

	return 0;
}

static void usage(char *program)
{
	printf("usage: %s <file> [<file> ...]\n", basename(program));
}

int main(int argc, char *argv[])
{
	Uint32 rmask, gmask, bmask, amask;
	unsigned long file_idx, nr_files;
	SDL_Surface *ansi_surface;
	struct bitmap_font *font;
	struct sauce_info *sauce;
	char window_caption[256];
	struct xbin_image *xbin;
	SDL_Surface *screen;
	SDL_Rect ansi_rect;
	SDL_Event ev;
	Uint32 flags;
	FILE *input;

	if (argc < 2) {
		usage(argv[0]);
		exit(1);
	}

	nr_files = argc - 1;

	font = (void *) cp437_font;

	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		die("Unable to initialize SDL");

	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0x00000000;

	ansi_surface = SDL_CreateRGBSurfaceFrom(
				mem,
				BUFFER_WIDTH,
				BUFFER_HEIGHT,
				BUFFER_DEPTH,
				BUFFER_WIDTH * BUFFER_DEPTH / 8,
				rmask, gmask, bmask, amask);
	if (!ansi_surface)
		die("Unable to create SDL RBG surface");

	flags = SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL | SDL_DOUBLEBUF;

	file_idx = 0;

restart:
	memset(mem, 0, sizeof(mem));

	input = fopen(argv[file_idx + 1], "r");
	if (!input)
		die("Unable to open file");

	xbin = xbin_load_image(fileno(input));
	if (xbin) {
		struct bitmap_font *xbin_font = (void *) xbin->xb_font->Font;
		int row, col;

		for (row = 0; row < xbin->xb_header->Height; row++) {
			for (col = 0; col < xbin->xb_header->Width; col++) {
				uint32_t fg_rgba, bg_rgba;
				unsigned long offset;
				uint8_t ch, attr;

				offset = (row * xbin->xb_header->Width + col) * 2;

				ch = xbin->xb_data[offset];
				attr = xbin->xb_data[offset + 1];

				if (xbin->xb_palette) {
					fg_rgba = xbin_palette_rgba(xbin->xb_palette, (attr & 0x0f));
					bg_rgba = xbin_palette_rgba(xbin->xb_palette, (attr & 0xf0) >> 4);
				} else {
					fg_rgba = attr_color_to_rgba((attr & 0x08), (attr & 0x0e));
					bg_rgba = attr_color_to_rgba(NORMAL, (attr & 0xf0) >> 4);
				}

				draw_char(xbin_font, col, row, fg_rgba, bg_rgba, ch);
			}
		}
	} else
		load_img(input, font);

	fseek(input, 0, SEEK_SET);

	sauce = sauce_info_read(input);

	if (sauce)
		snprintf(window_caption, sizeof window_caption, "%s - %s / %s - DuhView",
			sauce->workname, sauce->author, sauce->group);
	else
		snprintf(window_caption, sizeof window_caption, "DuhView");

	fclose(input);

	SDL_WM_SetCaption(window_caption, window_caption);

	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH, flags);
	if (!screen)
		die("Unable to set SDL video mode");

	SDL_EnableKeyRepeat(200, 50);

	ansi_rect.x = 0;
	ansi_rect.y = 0;
	ansi_rect.w = SCREEN_WIDTH;
	ansi_rect.h = SCREEN_HEIGHT;

	for (;;) {
		SDL_BlitSurface(ansi_surface, &ansi_rect, screen, NULL);
		SDL_Flip(screen);

		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				goto exit;
			case SDL_KEYDOWN: {
				switch (ev.key.keysym.sym) {
				case SDLK_HOME:
					ansi_rect.y = 0;
					break;
				case SDLK_UP:
					if (ansi_rect.y > CHAR_HEIGHT)
						ansi_rect.y -= CHAR_HEIGHT;
					else
						ansi_rect.y = 0;
					break;
				case SDLK_DOWN:
					ansi_rect.y += CHAR_HEIGHT;
					break;
				case SDLK_PAGEUP:
					if (ansi_rect.y > SCREEN_HEIGHT)
						ansi_rect.y -= SCREEN_HEIGHT;
					else
						ansi_rect.y = 0;
					break;
				case SDLK_SPACE:
				case SDLK_PAGEDOWN:
					ansi_rect.y += SCREEN_HEIGHT;
					break;
				case SDLK_ESCAPE:
					goto exit;
				case SDLK_LEFT:
					if (file_idx > 0)
						file_idx--;
					else
						file_idx = nr_files-1;
					goto restart;
				case SDLK_RIGHT:
					if (file_idx < (nr_files-1))
						file_idx++;
					else
						file_idx = 0;
					goto restart;
				default:
					break;
				}
			}
			default:
				break;
			}
		}

		SDL_Delay(1000 / 60);
	}
exit:
	SDL_VideoQuit();
	SDL_FreeSurface(ansi_surface);
	SDL_Quit();

	if (sauce)
		sauce_info_delete(sauce);

	return  0;
}
