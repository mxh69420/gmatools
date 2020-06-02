#pragma once
#include <stdint.h>
#include <stddef.h>

#define GMA_MAX_VERSION	3

struct gma_header {
	size_t length; //the body of the archive is at (pointer to buf) + length

	char ident[4];
	char version;

	uint64_t steamid;
	uint64_t timestamp;

	const char *content;

	const char *name;
	const char *desc;
	const char *author;

	int32_t addon_version;
};

int gma_header_parse(struct gma_header *, const void *, size_t);
/* uses standard errno codes for error handling
 * ENODATA: unexpected EOF
 * EPROTO: not a gma archive
 * ENOTSUP: version higher than GMA_MAX_VERSION

 * stored in errno
*/
