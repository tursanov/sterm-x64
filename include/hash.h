/* ����� � ���樠⨢�묨 ���ᨢ���. (c) gsr 2000, 2003, 2009 */

#if !defined HASH_H
#define HASH_H

#if defined __cplusplus
extern "C" {
#endif

#include "sysdefs.h"
#include "sterm.h"

#define HASH_ID_MIN		0x30
#define HASH_ID_MAX		0x7e
#define HASH_SIZE		HASH_ID_MAX - HASH_ID_MIN + 1
#define HASH_DELIM		0x1c
#define HASH_IDELIM		0x00	/* ����७��� ࠧ����⥫� ����ᥩ */
#define IS_HASH_ID(c) (bool)(((c) >= HASH_ID_MIN) && ((c) <= HASH_ID_MAX))

/*
 * ���樠⨢�� ���ᨢ. ������ ��室���� �� �� �������⮢�� �����䨪����
 * ����� �࠭���� � ���� <id> <data...> <0x00>
 */
struct hash{
	uint8_t *data;		/* ������� ������ */
	int hash_len;		/* ����� ������ ������ */
	int data_len;		/* ����� ������ */
};

extern struct hash	*create_hash(int l);
extern void		release_hash(struct hash *h);
extern void		clear_hash(struct hash *h);
/* �㭪�� �ᯮ������ ��� ��⠭���� ���祭�� �� 㬮�砭�� � ��� ����⠭� */
extern void		set_hash_defaults(struct hash *h);
extern bool		in_hash(struct hash *h, uint8_t id);
extern int		hash_get_len(struct hash *h, uint8_t id);
extern int		hash_get(struct hash *h, uint8_t id, uint8_t *buf);
extern bool		hash_set(struct hash *h, uint8_t id, uint8_t *txt);
extern int		hash_set_all(struct hash *h, uint8_t *str);

#if defined __cplusplus
}
#endif

#endif		/* HASH_H */
