#include <stdio.h>
#include <string.h>

struct string {
	int len;
	char *addr;
};

void std_print(struct string *s)
{
	printf("%.*s", s->len, s->addr);
}

void std_print_int(int i)
{
	printf("%i", i);
}

void std_print_float(float f)
{
	printf("%f", f);
}

int favorite_number()
{
	return 47;
}

extern int main_entry_point(int argc, struct string *argv);

int main(int argc, char **argv)
{
	struct string sargv[argc];
	for (int i = 0; i < argc; i++) {
		sargv[i].len = strlen(argv[i]);
		sargv[i].addr = argv[i];
	}
	return main_entry_point(argc, sargv);
}
