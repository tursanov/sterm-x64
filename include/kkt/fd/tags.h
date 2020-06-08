#ifndef TAGS_H
#define TAGS_H

#include <stdint.h>
#include "sysdefs.h"
#include "kkt/fd/tlv.h"

typedef enum {
	tag_type_byte,
	tag_type_uint16,
	tag_type_uint32,
	tag_type_unixtime,
	tag_type_string,
	tag_type_vln,
	tag_type_fvln,
	tag_type_bits,
	tag_type_bytes,
	tag_type_stlv
} tag_type_t;

typedef struct kkt_tag_t {
	uint16_t tag;
	const char *desc;
	tag_type_t type;
} kkt_tag_t;

const char *tags_get_text(uint16_t tag);
tag_type_t tags_get_tlv_text(ffd_tlv_t *tlv, char *text, size_t text_size);

#endif // TAGS_H
