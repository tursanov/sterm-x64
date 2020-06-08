#include "sysdefs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "kkt/fd/tlv.h"

int ffd_tlv_vln_length(uint64_t value)
{
	if (value <= 0xffll)
		return 1;
	if (value <= 0xffffll)
		return 2;
	if (value <= 0xffffffll)
		return 3;
	if (value <= 0xffffffffll)
		return 4;
	if (value <= 0xffffffffffll)
		return 5;
	if (value <= 0xffffffffffffll)
		return 6;
	if (value <= 0xffffffffffffffll)
		return 7;
	if (value <= 0xffffffffffffffffll)
		return 8;
	return -1;
}

#define FFD_TLV_MAX_SIZE	32768

static uint8_t tlv_data[FFD_TLV_MAX_SIZE];
static uint32_t tlv_size = 0;
static ffd_tlv_t *tlv = (ffd_tlv_t *)tlv_data;

#define TLV_DATA()	((uint8_t *)(tlv + 1))

typedef struct {
	ffd_tlv_t *tlv;
	uint16_t max_length;
} ffd_stlv_t;

#define STLV_MAX_LEVEL	4
static ffd_stlv_t stlvs[STLV_MAX_LEVEL];
static ffd_stlv_t* max_stlv = &stlvs[STLV_MAX_LEVEL - 1];
static ffd_stlv_t* stlv = NULL;

uint8_t *ffd_tlv_data() {
	return !stlv ? tlv_data : NULL;
}

size_t ffd_tlv_size() {
	return tlv_size;
}

void ffd_tlv_reset()
{
	tlv_size = 0;
	tlv = (ffd_tlv_t *)tlv_data;
	stlv = NULL;
}

int ffd_stlv_add_length(size_t size) {
	if (stlv) {
		stlv->tlv->length += size;
		if (stlv->tlv->length > stlv->max_length)
			return -1;
	}
	return 0;
}

int ffd_tlv_check_size(size_t size) {
	size_t curr_tlv_size = sizeof(ffd_tlv_t) + size;
	if (tlv_size + curr_tlv_size > FFD_TLV_MAX_SIZE)
		return -1;
	if (ffd_stlv_add_length(curr_tlv_size))
		return -2;
	return curr_tlv_size;
}

void ffd_tlv_commit(int curr_tlv_size) {
	tlv_size += curr_tlv_size;
	tlv = (ffd_tlv_t *)(tlv_data + tlv_size);
}

void *ffd_tlv_prepare(uint16_t tag, size_t size)
{
	int curr_tlv_size = ffd_tlv_check_size(size);
	if (curr_tlv_size < 0)
		return NULL;

	tlv->tag = tag;
	tlv->length = (uint16_t)size;
	void* prev = TLV_DATA();

	ffd_tlv_commit(curr_tlv_size);
	return prev;
}

int ffd_tlv_add(ffd_tlv_t *tlv) {
	uint8_t* data = (uint8_t*)ffd_tlv_prepare(tlv->tag, tlv->length);
	if (!data)
		return -1;
	memcpy(data, FFD_TLV_DATA(tlv), tlv->length);
	return 0;
}

int ffd_tlv_add_uint8(uint16_t tag, uint8_t value)
{
	uint8_t* data = (uint8_t*)ffd_tlv_prepare(tag, sizeof(value));
	if (!data)
		return -1;
	*data = value;
	return 0;
}

int ffd_tlv_add_uint16(uint16_t tag, uint16_t value)
{
	uint16_t* data = (uint16_t*)ffd_tlv_prepare(tag, sizeof(value));
	if (!data)
		return -1;
	*data = value;
	return 0;
}

int ffd_tlv_add_uint32(uint16_t tag, uint32_t value)
{
	uint32_t* data = (uint32_t*)ffd_tlv_prepare(tag, sizeof(value));
	if (!data)
		return -1;
	*data = value;
	return 0;
}

int ffd_tlv_add_unix_time(uint16_t tag, time_t value)
{
	return ffd_tlv_add_uint32(tag, (uint32_t)value);
}

int ffd_tlv_add_byte_array(uint16_t tag, uint8_t* value, size_t size)
{
	void* data = ffd_tlv_prepare(tag, size);
	if (!data)
		return -1;
	memcpy(data, value, size);
	return 0;
}

/*uint8_t win_dos_tab[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x3F, 0x3F, 0x27, 0x3F, 0x22, 0x3A, 0xC5, 0xD8, 0x3F, 0x25, 0x3F, 0x3C, 0x3F, 0x3F, 0x3F, 0x3F,
	0x3F, 0x27, 0x27, 0x22, 0x22, 0x07, 0x2D, 0x2D, 0x3F, 0x54, 0x3F, 0x3E, 0x3F, 0x3F, 0x3F, 0x3F,
	0xFF, 0xF6, 0xF7, 0x3F, 0xFD, 0x3F, 0xB3, 0x15, 0xF0, 0x63, 0xF2, 0x3C, 0xBF, 0x2D, 0x52, 0xF4,
	0xF8, 0x2B, 0x3F, 0x3F, 0x3F, 0xE7, 0x14, 0xFA, 0xF1, 0xFC, 0xF3, 0x3E, 0x3F, 0x3F, 0x3F, 0xF5,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
};

char *win_dos(char *buffer, size_t size) {
	char *ptr = buffer;
	for (size_t i = 0; i < size; i++,ptr++)
		*ptr = (char)win_dos_tab[(uint8_t)*ptr];
	return (buffer);
}
*/

int ffd_tlv_add_string(uint16_t tag, const char* value)
{
	if (value == NULL)
		return -1;
	uint16_t tlv_length = strlen(value);

	int curr_tlv_size = ffd_tlv_check_size(tlv_length);
	if (curr_tlv_size < 0)
		return -1;

	tlv->tag = tag;
	tlv->length = tlv_length;
	if (tlv_length > 0) {
		memcpy(TLV_DATA(), value, tlv_length);
		//win_dos(TLV_DATA(), strsize);
	}
	ffd_tlv_commit(curr_tlv_size);

	return 0;
}

int ffd_tlv_add_fixed_string(uint16_t tag, const char* value, size_t fixed_length)
{
	if (value == NULL)
		return -1;
	uint16_t length = strlen(value);
	if (length > fixed_length)
		length = fixed_length;

	int curr_tlv_size = ffd_tlv_check_size(fixed_length);
	if (curr_tlv_size < 0)
		return -1;

	tlv->tag = tag;
	tlv->length = fixed_length;
	if (length > 0)
		memcpy(TLV_DATA(), value, length);
	if (length < fixed_length)
		memset(TLV_DATA() + length, ' ', fixed_length - length);

	ffd_tlv_commit(curr_tlv_size);

	return 0;
}

int ffd_tlv_add_vln(uint16_t tag, uint64_t value)
{
	int len;
	if (tlv_size + sizeof(ffd_tlv_t) + 8 > FFD_TLV_MAX_SIZE)
		return -1;
	tlv->tag = tag;
	if ((len = ffd_tlv_vln_length(value)) > 6 || len < 0)
		return -2;
	tlv->length = (uint16_t)len;

	*(uint64_t *)TLV_DATA() = value;

	size_t curr_tlv_size = tlv->length + sizeof(ffd_tlv_t);
	if (ffd_stlv_add_length(curr_tlv_size))
		return -1;
	ffd_tlv_commit(curr_tlv_size);

	return 0;
}

int ffd_tlv_add_fvln(uint16_t tag, uint64_t value, uint8_t dot_pos)
{
	uint8_t *ptr;
	int len;
	if (tlv_size + sizeof(ffd_tlv_t) + 9 > FFD_TLV_MAX_SIZE)
		return -1;
	tlv->tag = tag;

	if ((len = ffd_tlv_vln_length(value)) > 7 || len < 0)
		return -2;

	tlv->length = (uint16_t)(len + 1);

	ptr = TLV_DATA();
	*ptr++ = dot_pos;

	*(uint64_t *)ptr = value;

	size_t curr_tlv_size = tlv->length + sizeof(ffd_tlv_t);
	if (ffd_stlv_add_length(curr_tlv_size))
		return -1;

	ffd_tlv_commit(curr_tlv_size);

	return 0;
}

int ffd_tlv_stlv_begin(uint16_t tag, uint16_t max_length)
{
	if (stlv == max_stlv)
		return -1;

	tlv->tag = tag;
	tlv->length = 0;

	if (!stlv)
		stlv = &stlvs[0];
	else
		stlv++;
	stlv->tlv = tlv;
	stlv->max_length = max_length;

	ffd_tlv_commit(sizeof(ffd_tlv_t));

	return 0;
}

int ffd_tlv_stlv_end()
{
	if (stlv != NULL) {
		if (stlv->tlv->length > stlv->max_length)
			return -1;
		if (stlv == stlvs)
			stlv = NULL;
		else
			stlv--;
	} else
		return -1;

	return 0;
}

double ffd_fvln_to_double(const ffd_fvln_t *fvln) {
	double v = fvln->value;
	for (int i = 0; i < fvln->dot; i++)
		v /= 10.0;
	return v;
}


bool ffd_string_to_fvln(const char *s, size_t size, ffd_fvln_t *value) {
	uint64_t v = 0;
	uint64_t m = 1;
	int dot = -1;
	int i = size - 1;

	s += i;
	for (; i >= 0; s--, i--) {
		if (*s == '.') {
			if (dot >= 0)
				return false;
			dot = size - i - 1;
		} else if (!isdigit(*s)) 
			return false;
		else {
			v += (*s - '0') * m;
			m *= 10;
		}
	}

	value->value = v;
	value->dot = dot < 0 ? 0 : dot;

	return true;
}

bool ffd_string_to_vln(const char *s, size_t size, uint64_t *value) {
	ffd_fvln_t v;
	if (!ffd_string_to_fvln(s, size, &v) || v.dot > 2)
		return false;

	if (v.dot == 2)
		*value = v.value;
	else if (v.dot == 1)
		*value = v.value * 10;
	else
		*value = v.value * 100;

	if (*value > MAX_VLN)
		return false;

	return true;
}

uint64_t ffd_tlv_data_as_vln(const ffd_tlv_t *tlv) {
	uint64_t v = 0;
	memcpy(&v, FFD_TLV_DATA(tlv), tlv->length);
	return v;
}

int ffd_tlv_data_as_fvln(const ffd_tlv_t *tlv, ffd_fvln_t *value) {
	uint8_t *p = FFD_TLV_DATA(tlv);
	value->dot = p[0];
	value->value = 0;
	memcpy(&value->value, p + 1, tlv->length - 1);
	return 0;
}
