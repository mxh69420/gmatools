#include "gma.h"
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

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
	printf("\x1b[m");

	return 0;
}
