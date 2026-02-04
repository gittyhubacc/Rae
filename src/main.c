#include "raec.h"
#include <mf/memory.h>
#include <mf/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct program_input {
        string src;
	cgopt opt;
};

#define PART_SZ (kibibyte / 2)
static struct program_input read_input(arena *a, int argc, char **argv)
{
        char c = 0;
        int parts_read = 0;
        int bytes_read = 0;
        char *buffer = make(a, char, PART_SZ);

        while ((c = fgetc(stdin)) != EOF) {
                buffer[(parts_read * PART_SZ) + bytes_read++] = c;
                if (bytes_read >= PART_SZ) {
                        steal(a, char, PART_SZ);
                        bytes_read = 0;
                        parts_read++;
                }
        }

        struct program_input i = {
            .src =
                (string){
                    .addr = buffer,
                    .len = parts_read * PART_SZ + bytes_read,
                },
	    .opt = {
		    .optimize = 1,
		    .target = LELNUX64,
		    .dest_path = S_NULL
	    }
        };

	int argi = 1;
	while (argi < argc) {
		if (strncmp(argv[argi], "-optimize", 2) == 0) {
			i.opt.optimize = strncmp(argv[argi + 1], "false", 5) == 0 ? 0 : 1;
			argi += 2;
			continue;
		}
		if (strncmp(argv[argi], "-target", 2) == 0) {
			if (strncmp(argv[argi + 1], "wasm", 4) == 0) {
				i.opt.target = WASM32;
			}
			if (strncmp(argv[argi + 1], "linux", 5) == 0) {
				i.opt.target = LELNUX64;
			}
			argi += 2;
			continue;
		}
		break;
	}

	if (argi < argc) {
                i.opt.dest_path = (string){
                    .addr = argv[argi],
                    .len = strlen(argv[argi]),
                };
        }

        return i;
}

int main(int argc, char **argv)
{
        int root_sz = 1048 * mebibyte;
        char *root_mem = malloc(root_sz);
        char *tmp_mem = malloc(root_sz);
        arena root = make_arena_ptr(root_mem, root_sz);
        arena tmp = make_arena_ptr(tmp_mem, root_sz);

        struct program_input in = read_input(&root, argc, argv);

        int code = raec(&root, tmp, in.src, in.opt);

	// reporting
	long used = ((long)bytes_used(root)) / kibibyte;
	long available = root_sz / kibibyte;
	char *units = "kibibytes";
	if (used > kibibyte * 10) {
		used /= kibibyte;
		available /= kibibyte;
		units = "mebibytes";
	}
	printf("%li of %li %s in use at exit\n", used, available, units);

        return 0;
}
