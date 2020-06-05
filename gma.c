#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

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

//for the header writing function, reckons how many bytes it will need
inline size_t gma_header_get_buffer_size(const struct gma_header *hdr){
	return
		sizeof(hdr->ident)	+
		sizeof(hdr->version)	+
		sizeof(hdr->steamid)	+
		sizeof(hdr->timestamp)	+
		strlen(hdr->name)	+ 1 +
		strlen(hdr->desc)	+ 1 +
		strlen(hdr->author)	+ 1 +
		sizeof(hdr->addon_version);
}

void *gma_header_write(const struct gma_header *, void *);
/* writes a header to buf
 * returns 1 past the end of the header
 * never fails, can only segfault if you pass it a too short buffer
*/

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

inline size_t gma_entry_get_buffer_size(struct gma_entry *e){
	return
		strlen(e->name) + 1 +
		sizeof(e->size) +
		sizeof(e->crc)  +
		sizeof(e->num);
}

void *gma_entry_write(const struct gma_entry *, void *);
beau@h:~/cpp/gmatools$ cat gma.c
#include "gma.h"

#include <string.h>
#include <errno.h>
#include <endian.h>

static inline int bufcpy(const void **begin, const void *end,
	void *dest, size_t count){

	if(end - *begin < count) return -1;
	memcpy(dest, *begin, count);
	*begin += count;
	return 0;
}

static inline const char *bufstr(const void **begin, const void *end){
	const void *const b = *begin;
	const size_t sl = strnlen(b, end - b);
	if(((const char *) b)[sl] != '\0') return NULL;

	*begin += sl;
	return b;
}

static inline int skip_null(const void **begin, const void *end){
	if(*begin + 1 >= end) return -1;
	*begin += 1;
	return 0;
}

static int _gma_header_parse(struct gma_header *hdr, const void *_buf, size_t count){
	const void *begin = _buf;
	const void *end = begin + count;

	const void *it = begin;

	//load hdr->ident and make sure it contains "GMAD"
	if(bufcpy(&it, end, hdr->ident, 4)) return -ENODATA;
	if(memcmp(hdr->ident, "GMAD", 4) != 0) return -EPROTO;

	//load version and make sure its supported
	if(bufcpy(&it, end, &hdr->version, 1)) return -ENODATA;
	if(hdr->version > GMA_MAX_VERSION) return -ENOTSUP;

	//steamid and timestamp
	if(bufcpy(&it, end, &hdr->steamid, 8)) return -ENODATA;
	if(bufcpy(&it, end, &hdr->timestamp, 8)) return -ENODATA;

	//fun fact: gmad doesnt do endian swapping for its integers. but i will!
	hdr->steamid = le64toh(hdr->steamid);
	hdr->timestamp = le64toh(hdr->timestamp);

	//version 1 and later have a string called "content" so ill read it out
	if(hdr->version > 1){
		hdr->content = bufstr(&it, end);
		if(hdr->content == NULL) return -ENODATA;
		if(skip_null(&it, end)) return -ENODATA;
	} else hdr->content = NULL;

	//load name
	hdr->name = bufstr(&it, end);
	if(hdr->name == NULL) return -ENODATA;
	if(skip_null(&it, end)) return -ENODATA;

	//load desc
	hdr->desc = bufstr(&it, end);
	if(hdr->desc == NULL) return -ENODATA;
	if(skip_null(&it, end)) return -ENODATA;

	//load author
	hdr->author = bufstr(&it, end);
	if(hdr->author == NULL) return -ENODATA;
	if(skip_null(&it, end)) return -ENODATA;

	//load addon version
	if(bufcpy(&it, end, &hdr->addon_version, 4)) return -ENODATA;

	hdr->length = it - begin;
	return 0;
}

int gma_header_parse(struct gma_header *hdr, const void *buf, size_t count){
	const int ret = _gma_header_parse(hdr, buf, count);
	if(ret == 0){
		return 0;
	} else {
		errno = -ret;
		return -1;
	}
}

static int _gma_iter(struct gma_iter *iter, struct gma_entry *e){
	//load file number, check if we hit file 0 (end of filenames)
	if(bufcpy(&iter->it, iter->end, &e->num, 4)) return -ENODATA;
	e->num = le32toh(e->num);

	if(e->num == 0) return 0;
	//past this point, there is more data

	//load file name
	e->name = bufstr(&iter->it, iter->end);
	if(e->name == NULL) return -ENODATA;
	if(skip_null(&iter->it, iter->end)) return -ENODATA;

	//load file size and crc
	if(bufcpy(&iter->it, iter->end, &e->size, 8)) return -ENODATA;
	e->size = le64toh(e->size);
	if(bufcpy(&iter->it, iter->end, &e->crc, 4)) return -ENODATA;
	e->crc = le32toh(e->crc);

	e->offset = iter->offset;
	iter->offset += e->size;

	return 1;
}

int gma_iter(struct gma_iter *iter, struct gma_entry *e){
	const int ret = _gma_iter(iter, e);
	if(ret < 0){
		errno = -ret;
		return -1;
	}
	return ret;
}

void *gma_header_write(const struct gma_header *hdr, void *buf){
	void *iter = buf;

	memcpy(iter, hdr->ident, 4);
	iter += 4;

	memcpy(iter, &hdr->version, 1);
	iter += 1;

	uint64_t steamid = hdr->steamid;
	steamid = htole64(steamid);
	memcpy(iter, &steamid, 8);
	iter += 8;

	uint64_t timestamp = hdr->timestamp;
	timestamp = htole64(timestamp);
	memcpy(iter, &timestamp, 8);
	iter += 8;

	if(hdr->version >= 1){
		size_t len = strlen(hdr->content) + 1;
		memcpy(iter, hdr->content, len);
		iter += len;
	}

	size_t len = strlen(hdr->name) + 1;
	memcpy(iter, hdr->name, len);
	iter += len;

	len = strlen(hdr->desc) + 1;
	memcpy(iter, hdr->desc, len);
	iter += len;

	len = strlen(hdr->author) + 1;
	memcpy(iter, hdr->author, len);
	iter += len;

	int32_t addon_version = hdr->addon_version;
	addon_version = htole32(addon_version);
	memcpy(iter, &addon_version, 4);
	iter += 4;

	return iter;
}

void *gma_entry_write(const struct gma_entry *e, void *buf){
	void *iter = buf;

	uint32_t num = e->num;
	num = htole32(num);
	memcpy(iter, &num, 4);
	iter += 4;

	size_t len = strlen(e->name) + 1;
	memcpy(iter, e->name, len);
	iter += len;

	uint64_t size = e->size;
	size = htole64(size);
	memcpy(iter, &size, 8);
	iter += 8;

	uint32_t crc = e->crc;
	crc = htole32(crc);
	memcpy(iter, &crc, 4);
	iter += 4;

	return iter;
}
