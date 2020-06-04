//not the best for addons with binary data, which is a lot of them

#include "gma.h"
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <stdlib.h>

struct mylist {
	struct mylist *prev;
	struct gma_entry e;
};

int main(int argc, const char **argv){
	if(argc < 2){
		fprintf(stderr, "usage: %s (file.gma)\n", argv[0]);
		return 1;
	}

	const int fd = open(argv[1], O_RDONLY);
	if(fd < 0){
		perror("open");
		return 1;
	}

	struct stat sb;
	if(fstat(fd, &sb)){
		perror("fstat");
		return 1;
	}

	const size_t sz = sb.st_size;

	const void *buf = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fd, 0);
	if(buf == MAP_FAILED){
		perror("mmap");
		return 1;
	}

	struct gma_header hdr;
	if(gma_header_parse(&hdr, buf, sz)){
		perror("gma_header_parse");
		return 1;
	}

	printf("begin header\n");

	printf("\x1b[36mident:\t\t\x1b[33m%.4s\n", hdr.ident);
	printf("\x1b[36mversion:\t\x1b[33m%hhd\n", hdr.version);

	printf("\x1b[36msteamid:\t\x1b[33m%lu\n", hdr.steamid);
	printf("\x1b[36mtimestamp:\t\x1b[33m%lu\n", hdr.timestamp);

	if(hdr.content != NULL)
		printf("\x1b[36mcontent:\t\x1b[33m%s\n", hdr.content);

	printf("\x1b[36mname:\t\t\x1b[33m%s\n", hdr.content);
	printf("\x1b[36mdesc:\x1b[33m\t\tbegin json:\n%s\n", hdr.desc);
	printf("\x1b[36mauthor:\t\t\x1b[33m%s\n", hdr.author);

	printf("\x1b[36maddon_version:\t\x1b[33m%d\n", hdr.addon_version);
	printf("\x1b[36mpayload offset:\t\x1b[33m%lu\n", hdr.length);
	printf("\n\x1b[mbegin file listing\n");

	struct gma_iter iter;
	gma_iter_init_from_header(&iter, &hdr, buf, sz);

	struct mylist *l = NULL;
	for(;;){
		struct mylist *cur = malloc(sizeof(*l));
		if(cur == NULL){
			perror("\x1b[mmalloc");
			return 1;
		}
		cur->prev = l;
		l = cur;

		int n = gma_iter(&iter, &l->e);
		if(n == 0) break;
		if(n < 0){
			perror("\x1b[mgma_iter");
			return 1;
		}

		printf("\x1b[36mfor file \x1b[33m%u:\n", l->e.num);

		printf("\t\x1b[36mname:\t\x1b[33m%s\n", l->e.name);
		printf("\t\x1b[36msize:\t\x1b[33m%lu\n", l->e.size);
		printf("\t\x1b[36mcrc:\t\x1b[33m%u\n", l->e.crc);
		printf("\t\x1b[36moffset:\t\x1b[33m%lu\n", l->e.offset);
	}

	printf("\n\x1b[mbegin file contents");

	while(l != NULL){
		printf("\n\x1b[36mfor file \x1b[33m%s: \x1b[mbegin file contents\n", l->e.name);
		const void *const fbuf = gma_iter_get_file(&iter, &l->e);
		fwrite(fbuf, 1, l->e.size, stdout);

		l = l->prev;
	}

	printf("\x1b[m\n");

	//leak it all since were exiting
	//none of the library code uses malloc so no valgrind needed
	return 0;
}
