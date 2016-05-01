#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "util.h"

#define XDG_DIRECTORY "/usr/share/applications/"

char name[1024];
char exec[1024];
char *search_pattern;
int ismatch;

void process_line(char *line)
{
	char *p;

	p = strchr(line, '\n');
	if (p)
		*p = '\0';

	if (!strncmp("Name=", line, 5))
		strcpy(name, line + 5);
	if (!strncmp("Exec=", line, 5))
		strcpy(exec, line + 5);

	/* Remove %U, %f, etc at the end of Exec cmd */
	p = strchr(exec, '%');
	if (p)
		*p = '\0';

	if (search_pattern &&
	    !strncmp("Categories=", line, 11) &&
	    strstr(line, search_pattern))
		ismatch = 1;
}

void read_file(FILE *fp)
{
	char line[1024];

	ismatch = 0;
	while (fgets(line, sizeof(line), fp))
		process_line(line);

	if (!search_pattern || ismatch)
		printf("%s,%s\n", name, exec);
}

int parse_file(char *filename)
{
	FILE *fp;
	char fullname[4096];

	strncpy(fullname, XDG_DIRECTORY, strlen(XDG_DIRECTORY));
	strncpy(fullname + strlen(XDG_DIRECTORY), filename, strlen(filename) + 1);

	fp = fopen(fullname, "r");
	if (!fp)
		die("could not open file %s", filename);
	read_file(fp);
	fclose(fp);
}

int list_dir(const char *path)
{
	struct dirent *entry;
	DIR *dp;

	dp = opendir(path);
	if (!dp)
		die("could not open dir %s", path);

	while (entry = readdir(dp))
		parse_file(entry->d_name);

	closedir(dp);
	return 0;
}

int main(int argc, char **argv)
{
	if (argc > 1)
		search_pattern = strdup(argv[1]);
	else
		search_pattern = NULL;

	if (search_pattern)
		printf("/xdg/%s,^tag(xdg)\n", search_pattern);
	else
		printf("/xdg/all,^tag(xdg)\n");

	list_dir(XDG_DIRECTORY);

	return 0;
}