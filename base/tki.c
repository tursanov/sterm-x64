/* �⥭��/������ ���祢�� ���ଠ樨 � �ନ����. (c) gsr 2004, 2005 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "base64.h"
#include "genfunc.h"
#include "license.h"
#include "licsig.h"
#include "md5.h"
#include "tki.h"

/* ����� �஢�ન */
/* ���� tki ��⠭ � �஢�७ */
bool tki_ok = false;
/* �����㦥� USB-��� ��� ������� �ନ���� */
bool usb_ok = false;
/* �����㦥� dst-䠩� ��� �⮣� �ନ���� */
bool iplir_ok = false;
/* ������ �ࠢ���� 䠩� ������᪮� ��業��� */
bool bank_ok = false;

/* ���� ���祢�� ���ଠ樨 (�࠭���� � ����஢����� ����) */
struct term_key_info tki;
/* ��⠭�� 䠩� �ਢ離� USB-��᪠ */
struct md5_hash usb_bind = ZERO_MD5_HASH;
/* ��⠭�� 䠩� �ਢ離� ���祢�� ��� */
struct md5_hash iplir_bind = ZERO_MD5_HASH;

/* ��⠭�� 䠩� ������᪮� ��業��� */
struct md5_hash bank_license;

/* ���஢���� ������ */
void encrypt_data(uint8_t *p, int l)
{
	int i;
	if (p == NULL)
		return;
/* ���砫� ���㥬 横���᪨� xor-�� */
	for (i = 0; i < l; i++)
		p[(i + 1) % l] ^= p[i];
/* ��᫥ 祣� ���塞 ���⠬� ���訥 � ����訥 ���㡠��� */
	for (i = 0; i < l; i++)
		p[i] = swap_nibbles(p[i]);
}

/* �����஢�� ������ */
void decrypt_data(uint8_t *p, int l)
{
	int i;
	if (p == NULL)
		return;
/* ���塞 ���⠬� ���訥 � ����訥 ���㡠��� */
	for (i = 0; i < l; i++)
		p[i] = swap_nibbles(p[i]);
/* �����஢뢠�� 横���᪨� xor */
	for (i = l; i > 0; i--)
		p[i % l] ^= p[i - 1];
}

/* ���樠������ �������� tki */
static void init_tki(struct term_key_info *p)
{
/*
 * �᫨ ��-�� ��ࠢ������� ����� �������� ����� ���� ��������� �஬���⪨,
 * �, ��������� memset, ��� ���� ��������� ��ﬨ.
 */
	memset(p, 0, sizeof(*p));
	memset(p->srv_keys, 0, sizeof(p->srv_keys));
	memset(p->dbg_keys, 0, sizeof(p->dbg_keys));
	memset(p->tn, 0x30, sizeof(p->tn));
	get_md5((uint8_t *)p->srv_keys, xsizefrom(*p, srv_keys), &p->check_sum);
	encrypt_data((uint8_t *)p, sizeof(*p));
}

/* �⥭�� 䠩�� ���祢�� ���ଠ樨 */
bool read_tki(const char *path, bool create)
{
	bool ret = false;
	struct stat st;
	if (stat(path, &st) == -1){
		if (create){
			init_tki(&tki);
			ret = true;
		}else
			fprintf(stderr, "���� ���祢�� ���ଠ樨 �� ������.\n");
	}else{
		if (st.st_size != sizeof(tki))
			fprintf(stderr, "���� ���祢�� ���ଠ樨 ����� ������ ࠧ���.\n");
		else{
			int fd = open(path, O_RDONLY);
			if (fd == -1)
				fprintf(stderr, "�訡�� ������ 䠩�� ���祢�� ���ଠ樨 "
					"��� �⥭��: %m.\n");
			else{
				memset(&tki, 0, sizeof(tki));
				ssize_t l = read(fd, &tki, sizeof(tki));
				if (l == sizeof(tki))
					ret = true;
				else if (l == -1)
					fprintf(stderr, "�訡�� �⥭�� 䠩�� ���祢�� "
						"���ଠ樨: %m.\n");
				else
					fprintf(stderr, "�� 䠩�� ���祢�� ���ଠ樨 "
						"���⠭� %zd ���� ����� %zd.\n", l, sizeof(tki));
				close(fd);
			}
		}
	}
	return ret;
}

/* ������ 䠩�� ���祢�� ���ଠ樨 */
bool write_tki(const char *path)
{
	bool ret = false;
	int fd = creat(path, 0600);
	if (fd == -1)
		fprintf(stderr, "�訡�� ᮧ����� 䠩�� ���祢�� ���ଠ樨: %m.\n");
	else{
		ssize_t l = write(fd, &tki, sizeof(tki));
		if (l == sizeof(tki))
			ret = true;
		else if (l == -1)
			fprintf(stderr, "�訡�� ����� 䠩�� ���祢�� ���ଠ樨: %m.\n");
		else
			fprintf(stderr, "� 䠩� ���祢�� ���ଠ樨 ����ᠭ� "
				"%zd ���� ����� %zd.\n", l, sizeof(tki));
		close(fd);
	}
	return ret;
}

/* �⥭�� 䠩�� �ਢ離� */
bool read_bind_file(const char *path, struct md5_hash *md5)
{
	bool ret = false;
	if ((path == NULL) || (md5 == NULL))
		return ret;
	struct stat st;
	if (stat(path, &st) == -1)
		fprintf(stderr, "�訡�� ����祭�� ���ଠ樨 � 䠩�� %s: %m.\n", path);
	else if (st.st_size != sizeof(*md5))
		fprintf(stderr, "���� %s ����� ������ ࠧ���.\n", path);
	else{
		int fd = open(path, O_RDONLY);
		if (fd == -1)
			fprintf(stderr, "�訡�� ������ 䠩�� %s: %m.\n", path);
		else{
			ssize_t l = read(fd, md5, sizeof(*md5));
			if (l == sizeof(*md5))
				ret = true;
			else if (l == -1)
				fprintf(stderr, "�訡�� �⥭�� �� 䠩�� %s: %m.\n", path);
			else
				fprintf(stderr, "�� 䠩�� %s ��⠭� %zd ���� ����� %zd.\n",
					path, l, sizeof(*md5));
			close(fd);
		}
	}
	return ret;
}

/* ����ઠ 䠩�� ���祢�� ���ଠ樨 */
void check_tki(void)
{
	struct md5_hash md5, check_sum;
	struct term_key_info __tki = tki;
	tki_ok = false;
	decrypt_data((uint8_t *)&__tki, sizeof(__tki));
	get_md5((uint8_t *)__tki.srv_keys, xsizefrom(__tki, srv_keys), &md5);
	encrypt_data((uint8_t *)&__tki, sizeof(__tki));
	get_tki_field(&__tki, TKI_CHECK_SUM, (uint8_t *)&check_sum);
	tki_ok = cmp_md5(&md5, &check_sum);
}

/* �஢�ઠ 䠩�� �ਢ離� USB-��᪠ */
void check_usb_bind(void)
{
	term_number tn;
	uint8_t buf[32];
	int l;
	struct md5_hash md5;
	get_tki_field(&tki, TKI_NUMBER, (uint8_t *)tn);
	l = base64_encode((uint8_t *)tn, sizeof(tn), buf);
	encrypt_data(buf, l);
	get_md5(buf, l, &md5);
	usb_ok = cmp_md5(&md5, &usb_bind);
}

/* �஢�ઠ 䠩�� �ਢ離� ���祢�� ��� */
void check_iplir_bind(void)
{
	struct md5_hash md5, buf[2];
	buf[0] = iplir_check_sum;
	buf[1] = usb_bind;
	get_md5((uint8_t *)buf, sizeof(buf), &md5);
	iplir_ok = cmp_md5(&md5, &iplir_bind);
}

/* �஢�ઠ ��業��� ��� */
void check_bank_license(void)
{
	uint8_t buf[2 * TERM_NUMBER_LEN];
	uint8_t v1[64], v2[64];
	struct md5_hash md5;
	int i;
	bank_ok = false;
#if defined __CHECK_LIC_SIGNATURE__
	if (check_lic_signature(BANK_LIC_SIGNATURE_OFFSET, BANK_LIC_SIGNATURE))
		return;		/* ��業��� �뫠 㤠���� */
#endif
	get_tki_field(&tki, TKI_NUMBER, buf);
	for (i = 0; i < TERM_NUMBER_LEN; i++)
		buf[sizeof(buf) - i - 1] = ~buf[i];
	i = base64_encode(buf, sizeof(buf), v1);
	i = base64_encode(v1, i, v2);
	get_md5((uint8_t *)v2, i, &md5);
	bank_ok = cmp_md5(&md5, &bank_license);
}

/* �⥭�� ��������� ���� �� �������� tki */
bool get_tki_field(const struct term_key_info *info, int name, uint8_t *val)
{
	bool ret = true;
	decrypt_data((uint8_t *)info, sizeof(*info));
	switch (name){
		case TKI_CHECK_SUM:
			memcpy(val, &info->check_sum, sizeof(info->check_sum));
			break;
		case TKI_SRV_KEYS:
			memcpy(val, info->srv_keys, sizeof(info->srv_keys));
			break;
		case TKI_DBG_KEYS:
			memcpy(val, info->dbg_keys, sizeof(info->dbg_keys));
			break;
		case TKI_NUMBER:
			memcpy(val, info->tn, sizeof(info->tn));
			break;
		default:
			ret = false
	}
	encrypt_data((uint8_t *)info, sizeof(*info));
	return ret;
}

/* ��⠭���� ���祭�� ��������� ���� �������� tki */
void set_tki_field(struct term_key_info *info, int name, const uint8_t *val)
{
	bool ret = true;
	decrypt_data((uint8_t *)info, sizeof(*info));
	switch (name){
		case TKI_CHECK_SUM:
			memcpy(&info->check_sum, val, sizeof(info->check_sum));
			break;
		case TKI_SRV_KEYS:
			memcpy(info->srv_keys, val, sizeof(info->srv_keys));
			break;
		case TKI_DBG_KEYS:
			memcpy(info->dbg_keys, val, sizeof(info->dbg_keys));
			break;
		case TKI_NUMBER:
			memcpy(info->tn, val, sizeof(info->tn));
			break;
		default:
			ret = false;
	}
	get_md5((uint8_t *)info->srv_keys, xsizefrom(*info, srv_keys),
			&info->check_sum);
	encrypt_data((uint8_t *)info, sizeof(*info));
	return ret;
}
