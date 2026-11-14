#if !defined KKT_FD_TLV_H
#define KKT_FD_TLV_H

#include <stdint.h>
#include <time.h>

__BEGIN_DECLS

#pragma pack(push, 1)
typedef struct ffd_tlv_t {
	uint16_t tag;
	uint16_t length;
} ffd_tlv_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
	uint8_t dot;
	uint64_t value;
} ffd_fvln_t;
#pragma pack(pop)

#define MAX_VLN	1099511627776

#define FFD_TLV_SIZE(tlv) ((tlv)->length + sizeof(ffd_tlv_t))
#define FFD_TLV_NEXT(tlv) ((const ffd_tlv_t *)((uint8_t *)(tlv) + FFD_TLV_SIZE(tlv)))
#define FFD_TLV_DATA(tlv) ((uint8_t *)(((const ffd_tlv_t *)(tlv)) + 1))
#define FFD_TLV_DATA_AS_UINT8(tlv) *((uint8_t *)FFD_TLV_DATA(tlv))
#define FFD_TLV_DATA_AS_UINT16(tlv) *((uint16_t *)FFD_TLV_DATA(tlv))
#define FFD_TLV_DATA_AS_UINT32(tlv) *((uint32_t *)FFD_TLV_DATA(tlv))
#define FFD_TLV_DATA_AS_PUNIXTIME(tlv) ((time_t *)FFD_TLV_DATA(tlv))
#define FFD_TLV_DATA_AS_UNIXTIME(tlv) (*((time_t *)FFD_TLV_DATA(tlv)))
#define FFD_TLV_DATA_AS_BITS(tlv)  ((tlv)->length == 1 ? (uint32_t)*((uint8_t *)FFD_TLV_DATA(tlv)) : *((uint32_t *)FFD_TLV_DATA(tlv)))
#define FFD_TLV_DATA_AS_STRING(tlv) ((char *)FFD_TLV_DATA(tlv))
extern uint64_t ffd_tlv_data_as_vln(const ffd_tlv_t *tlv);
extern int ffd_tlv_data_as_fvln(const ffd_tlv_t *tlv, ffd_fvln_t *value);

extern double ffd_fvln_to_double(const ffd_fvln_t *value);
extern bool ffd_string_to_fvln(const char *s, size_t size, ffd_fvln_t *value);
extern bool ffd_string_to_vln(const char *s, size_t size, uint64_t *value);

extern int ffd_tlv_vln_length(uint64_t value);

extern uint8_t *ffd_tlv_data();
extern size_t ffd_tlv_size();
extern void ffd_tlv_reset();
extern int ffd_tlv_add(ffd_tlv_t *tlv);
extern int ffd_tlv_add_uint8(uint16_t tag, uint8_t value);
extern int ffd_tlv_add_uint16(uint16_t tag, uint16_t value);
extern int ffd_tlv_add_uint32(uint16_t tag, uint32_t value);
extern int ffd_tlv_add_vln(uint16_t tag, uint64_t value);
extern int ffd_tlv_add_fvln(uint16_t tag, uint64_t value, uint8_t dot_pos);
extern int ffd_tlv_add_unix_time(uint16_t tag, time_t value);
extern int ffd_tlv_add_byte_array(uint16_t tag, uint8_t* value, size_t size);
extern int ffd_tlv_add_string(uint16_t tag, const char* value);
extern int ffd_tlv_add_fixed_string(uint16_t tag, const char* value, size_t fixed_length);
extern int ffd_tlv_stlv_begin(uint16_t tag, uint16_t max_length);
extern int ffd_tlv_stlv_end();

__END_DECLS

#endif		/* KKT_FD_TLV_H */
