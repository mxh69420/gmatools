#pragma once
#include <stdint.h>
#include <stddef.h>

#define GMA_MAX_VERSION	3

struct gma_header {
	size_t length;	//the body of the archive is at (pointer to buf) + length
			//actually now you can do it with gma_header_get_payload
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

 * returns 0 on success
*/


//making things easy for you :)
inline const void *gma_header_get_payload(
	const struct gma_header *hdr, const void *buf){

	return buf + hdr->length;
}
inline size_t gma_header_get_payload_size(const struct gma_header *hdr,
	size_t sz){

	return sz - hdr->length;
}

struct gma_entry {
	const char *name;
	uint64_t size;
	uint32_t crc;
	uint32_t num;
	uintptr_t offset;
};

struct gma_iter {
	const void *begin;
	const void *it;
	const void *end;
	uintptr_t offset;
};

inline void gma_iter_init(struct gma_iter *iter, const void *buf, size_t sz){
	iter->begin = buf;
	iter->it = buf;
	iter->end = buf + sz;
	iter->offset = 0;
}

inline void gma_iter_init_from_header(struct gma_iter *iter,
	const struct gma_header *hdr, const void *buf, size_t sz){

	gma_iter_init(iter, gma_header_get_payload(hdr, buf),
		gma_header_get_payload_size(hdr, sz));
}

int gma_iter(struct gma_iter *, struct gma_entry *);
/* returns -1 on error, with errno set to ENODATA
 * returns 0 when its done
 * returns 1 when theres another entry
*/

inline const void *gma_iter_get_base(const struct gma_iter *iter){
	return iter->it;
}
/* returns a pointer to the file base. add offset from a gma_entry to this base
 * to get the data for the file.

 * this function is only right when all entries have been iterated over. else,
 * you will be reading header thinking that its part of a file and be like
 * "SAMMY FIX YOUR STUPID CODE WHY AM I GETTING HEADER DATA WHEN I TRY TO GET
 *  FILE CONTENTS??????????????"
*/

inline const void *gma_iter_get_file(
	const struct gma_iter *iter, const struct gma_entry *e){

	return gma_iter_get_base(iter) + e->offset;
}
// same case as gma_iter_get_base
// returns a pointer to the file that e references
