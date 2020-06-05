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
