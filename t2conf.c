/* Simple tint2rc parser for jgmenu */

#include <string.h>

#include "util.h"
#include "config.h"
#include "sbuf.h"
#include "t2conf.h"

#define DEFAULT_TINT2RC "~/.config/tint2/tint2rc"
#define DELIM " \t\r\n"

enum alignment {UNKNOWN, TOP, CENTER, BOTTOM, LEFT, RIGHT, HORIZONTAL,
		VERTICAL};

static int g_screen_height, g_screen_width;
static int bg_id;

#define MAX_NR_BGS (16)

struct bg {
	int rounded;
	int border_width;
	char *background_color;
	char *border_color;
};

/* bg[0] is used for fully transparent bg */
static struct bg bg[MAX_NR_BGS + 1];

static enum alignment valign = UNKNOWN;
static enum alignment halign = UNKNOWN;
static enum alignment orientation = UNKNOWN;
static char *panel_width;
static char *panel_height;
static int panel_margin_h;
static int panel_margin_v;

static int parse_height(const char *h)
{
	char *p;

	p = strchr(h, '%');
	if (!p)
		return atoi(h);
	p = '\0';
	return atoi(h) / 100.0 * g_screen_height;
}

static int parse_width(const char *w)
{
	char *p;

	p = strchr(w, '%');
	if (!p)
		return atoi(w);
	p = '\0';
	return atoi(w) / 100.0 * g_screen_width;
}

static void process_line(char *line)
{
	char *option, *value, *field;
	int id;

	if (!parse_config_line(line, &option, &value))
		return;

	if (!strcmp(option, "rounded")) {
		bg_id++;
		bg[bg_id].rounded = atoi(value);
	} else if (!strcmp(option, "border_width")) {
		bg[bg_id].border_width = atoi(value);
	} else if (!strcmp(option, "background_color")) {
		bg[bg_id].background_color = strdup(value);
	} else if (!strcmp(option, "border_color")) {
		bg[bg_id].border_color = strdup(value);

	} else if (!strncmp(option, "panel_background_id", 19)) {
		id = atoi(value);
		if (id > MAX_NR_BGS) {
			fprintf(stderr, "warn: id too big\n");
			return;
		}
		printf("color_menu_bg    = %s\n", bg[id].background_color);
		parse_hexstr(bg[id].background_color, config.color_menu_bg);

		/*
		 * We could parse set color_menu_border and border_width here,
		 * but choose not too because it sometimes looks a bit strange.
		 * parse_hexstr(bg[id].border_color, config.color_menu_border);
		 * config.menu_border = bg[id].border_width;
		 */

	} else if (!strncmp(option, "taskbar_background_id", 21)) {
		; /* ignore this */
	} else if (!strncmp(option, "task_background_id", 18)) {
		id = atoi(value);
		if (id > MAX_NR_BGS) {
			fprintf(stderr, "warn: id too big\n");
			return;
		}

		/*
		 * We don't set color_norm_bg. Only the selected item gets
		 * a colour. If we wanted it, it would be:
		 * parse_hexstr(bg[id].background_color, config.color_norm_bg);
		 */

	} else if (!strncmp(option, "task_active_background_id", 25)) {
		id = atoi(value);
		if (id > MAX_NR_BGS) {
			fprintf(stderr, "warn: id too big\n");
			return;
		}
		printf("item_radius      = %d\n", bg[id].rounded);
		config.item_radius = bg[id].rounded;
		printf("color_sel_bg     = %s\n", bg[id].background_color);
		parse_hexstr(bg[id].background_color, config.color_sel_bg);
		printf("color_sel_border = %s\n", bg[id].border_color);
		parse_hexstr(bg[id].border_color, config.color_sel_border);
		printf("item_border      = %d\n", bg[id].border_width);
		config.item_border = bg[id].border_width;

	} else if (!strcmp(option, "task_font_color")) {
		printf("color_norm_fg    = %s\n", value);
		parse_hexstr(value, config.color_norm_fg);
	} else if (!strcmp(option, "task_active_font_color")) {
		printf("color_sel_fg     = %s\n", value);
		parse_hexstr(value, config.color_sel_fg);
	} else if (!strcmp(option, "panel_position")) {
		field = strtok(value, DELIM);
		if (!field)
			return;
		if (!strcmp(field, "bottom"))
			valign = BOTTOM;
		else if (!strcmp(field, "top"))
			valign = TOP;
		else if (!strcmp(field, "center"))
			valign = CENTER;

		field = strtok(NULL, DELIM);
		if (!field)
			return;
		if (!strcmp(field, "left"))
			halign = LEFT;
		else if (!strcmp(field, "right"))
			halign = RIGHT;
		else if (!strcmp(field, "center"))
			halign = CENTER;

		field = strtok(NULL, DELIM);
		if (!field)
			return;
		if (!strcmp(field, "horizontal"))
			orientation = HORIZONTAL;
		else if (!strcmp(field, "vertical"))
			orientation = VERTICAL;

	} else if (!strcmp(option, "panel_size")) {
		/*
		 * For a vertical panel, panel_size's height/width are swapped
		 * We cannot calculate width/height at this point as we have
		 * to be able to resolve '%' and might not know alignment yet.
		 */
		field = strtok(value, DELIM);
		if (!field)
			return;
		panel_width = strdup(field);

		field = strtok(NULL, DELIM);
		if (!field)
			return;
		panel_height = strdup(field);

	} else if (!strcmp(option, "panel_margin")) {
		field = strtok(value, DELIM);
		if (!field)
			return;
		panel_margin_h = atoi(field);
		field = strtok(NULL, DELIM);
		if (!field)
			return;
		panel_margin_v = atoi(field);
	}
}

static void read_file(FILE *fp)
{
	char line[1024];

	while (fgets(line, sizeof(line), fp))
		process_line(line);
}

void parse_file(char *filename)
{
	FILE *fp;

	fp = fopen(filename, "r");
	if (!fp) {
		fprintf(stderr, "warning: cannot open file '%s'\n", filename);
		return;
	}

	printf("From tint2rc:\n");
	read_file(fp);
	fclose(fp);
}

void set_alignment_and_position(void)
{
	printf("orientation      = ");
	if (orientation == HORIZONTAL)
		printf("horizontal\n");
	else if (orientation == VERTICAL)
		printf("vertical\n");

	if (orientation == HORIZONTAL) {
		printf("margin_y         = %d\n", parse_height(panel_height) + panel_margin_v);
		printf("halign           = left\n");
		if (valign == TOP)
			printf("valign           = top\n");
		else if (valign == BOTTOM)
			printf("valign           = bottom\n");
	}

	if (orientation == VERTICAL) {
		printf("margin_x         = %d\n", parse_width(panel_height) + panel_margin_h);
		printf("valign           = top\n");
		if (halign == LEFT)
			printf("halign           = left\n");
		else if (halign == RIGHT)
			printf("halign           = right\n");
	}
}

void t2conf_cleanup(void)
{
	int i;

	for (i = 0; i < MAX_NR_BGS; i++) {
		if (bg[i].background_color)
			free(bg[i].background_color);
		if (bg[i].border_color)
			free(bg[i].border_color);
	}
	if (panel_width)
		free(panel_width);
	if (panel_height)
		free(panel_height);
}

void tint2rc_parse(const char *filename, int screen_width, int screen_height)
{
	struct sbuf tint2rc;

	g_screen_width = screen_width;
	g_screen_height = screen_height;
	bg[0].background_color = strdup("#000000 00");
	bg[0].border_color = strdup("#000000 00");
	sbuf_init(&tint2rc);
	if (filename)
		sbuf_addstr(&tint2rc, filename);
	else
		sbuf_addstr(&tint2rc, DEFAULT_TINT2RC);
	sbuf_expand_tilde(&tint2rc);
	parse_file(tint2rc.buf);
	free(tint2rc.buf);
	set_alignment_and_position();
	t2conf_cleanup();
}
