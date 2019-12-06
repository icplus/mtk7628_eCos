#include <stdio.h>

static int compile(char *txtfile);

int main(int argc, char* argv[])
{
	char		*txtfile;

	txtfile = NULL;

	txtfile = argv[1];

	if (compile(txtfile) < 0) {
		return -1;
	}
	return 0;
}



static int compile(char *txtfile)
{
	FILE			*lp;
	char			buf;
	int				len=0;

/*
 *	Open list of files
 */
	if ((lp = fopen(txtfile, "r")) == NULL) {
		fprintf(stderr, "Can't open file list %s\n", txtfile);
		return -1;
	}

	fprintf(stdout, "/*\n * profile.c *\n");
	fprintf(stdout, " * simple convert to array */\n\n"); 

 	fprintf(stdout, "unsigned char profile[] = {\n");
	while (fread(&buf, 1, 1, lp) != NULL) {
		len++;
		fprintf(stdout, "%3d,", buf);
	}
	fprintf(stdout, "    0 };\n\n");
	printf("int profile_len=%d;\n", len);

	fclose(lp);

	fflush(stdout);
	return 0;
}

/******************************************************************************/

