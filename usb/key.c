/* ����� � ���㫥� ������᭮�� VipNet. (c) gsr, Alex Popov 2004 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tki.h"
#include "usb/core.h"
#include "usb/key.h"

#define USB_KEY_NUMBER		0

/* ����� �� ���᮫� ��� ����প� �뢮�� */
__attribute__((format (printf, 1, 2))) static int xprintf(char *format, ...)
{
	int ret;
	va_list p;
	va_start(p, format);
	ret = vprintf(format, p);
	fflush(stdout);
	va_end(p);
	return ret;
}

/* ���� ��� ࠡ��� � USB-���箬 */
static uint8_t key_buf[USB_KEY_SIZE];
static struct key_header *key_hdr = (struct key_header *)key_buf;

#if 0
static void dump_data(uint8_t *data, int len)
{
	int i;
	if (data == NULL)
		return;
	for (i = 0; i < len; i++){
		printf(" %.2hx", (uint16_t)data[i]);
		if (i > 0){
			if ((i % 16) == 0)
				printf("\n");
			else if ((i % 8) == 0)
				printf(" --");
		}
	}
}
#endif

/* ������ ������ � 䠩� */
static bool write_to_file(int offset, int len, char *name)
{
	int fd;
	struct key_header *kh;
	struct key_data_header *kdh;
	if ((name == NULL) || (offset < sizeof(*kh)) ||
			(offset > (USB_KEY_SIZE - sizeof(*kdh))))
		return false;
	fd = creat(name, 0600);
	if (fd == -1){
		xprintf("�訡�� ᮧ����� 䠩�� %s: %s\n",
				name, strerror(errno));
		return false;
	}
	len -= write(fd, key_buf + offset, len);
	if (len != 0)
		xprintf("write(\"%s\") %d ����: %s", name, len,
				strerror(errno));
	close(fd);
	return len == 0;
}

/* �஢�ઠ ����� ������ � ���� */
static bool check_data_chunk(int offset, int expected_len)
{
	struct key_data_header *kdh;
	struct md5_hash md5;
	if ((offset < sizeof(struct key_header)) ||
			(offset > (key_hdr->data_size)))
		return false;
	kdh = (struct key_data_header *)(key_buf + offset);
	if (((offset + kdh->len) > USB_KEY_SIZE) ||
			((expected_len != -1) &&
			((kdh->len - sizeof(*kdh)) != expected_len))){
		xprintf("����୮� ���祭�� ���� ����� �� ᬥ饭�� %d\n", offset);
		return false;
	}
	get_md5(key_buf + offset + sizeof(*kdh), kdh->len - sizeof(*kdh), &md5);
	if (!cmp_md5(&md5, &kdh->crc)){
		xprintf("��ᮢ������� ����஫쭮� �㬬� ����� ������ "
				"�� ᬥ饭�� %d\n", offset);
		return false;
	}else
		return true;
}

/* �஢�ઠ ������ USB-���� */
static bool check_data(void)
{
	int offset = sizeof(struct key_header);
	struct key_data_header *kdh =
		(struct key_data_header *)(key_buf + sizeof(struct key_header));
/* ���� �ਢ離� ���祢��� ��᪠ */
	if (!check_data_chunk(offset, sizeof(struct md5_hash)) ||
			!write_to_file(offset + sizeof(*kdh),
				sizeof(struct md5_hash), USB_BIND_FILE))
		return false;
	if (key_hdr->n_chunks == 1)
		return true;
/* ���� �ਢ離� ���祢��� ����ਡ�⨢� */
	offset += kdh->len;
	kdh = (struct key_data_header *)(key_buf + offset);
	if (!check_data_chunk(offset, sizeof(struct md5_hash)) ||
			!write_to_file(offset + sizeof(*kdh),
				sizeof(struct md5_hash), IPLIR_BIND_FILE))
		return false;
/* ���� ��஫� ���祢��� ����ਡ�⨢� */
	offset += kdh->len;
	kdh = (struct key_data_header *)(key_buf + offset);
	if (!check_data_chunk(offset, -1) ||
			!write_to_file(offset + sizeof(*kdh),
				kdh->len - sizeof(*kdh), IPLIR_PSW_DATA))
		return false;
/* ���� ���祢��� ����ਡ�⨢� */
	offset += kdh->len;
	kdh = (struct key_data_header *)(key_buf + offset);
	return check_data_chunk(offset, -1) &&
			write_to_file(offset + sizeof(*kdh),
				kdh->len - sizeof(*kdh), IPLIR_DST);
}

/* �⥭�� ��������� USB-���� */
static bool read_key_header(void)
{
	bool ret = false;
	xprintf("�⥭�� ��������� ����� ������᭮��: ");
	if (!usbkey_open(USB_KEY_NUMBER)){
		xprintf("�訡�� ������ ����� ������᭮��\n");
		return false;
	}else if (!usbkey_read(USB_KEY_NUMBER, key_buf, 0, 1)){
		xprintf("�訡�� �⥭�� ��������� ����� ������᭮��\n");
		goto read_key_header_end;
	}
	if (key_hdr->magic != USB_KEY_MAGIC){
		xprintf("��������� ����� ������᭮�� ���०���\n");
		goto read_key_header_end;
	}else if (key_hdr->data_size > USB_KEY_SIZE){
		xprintf("������ ࠧ��� ������ ����� ������᭮�� (%d ����)\n",
				key_hdr->data_size);
		goto read_key_header_end;
	}else if ((key_hdr->n_chunks != 1) && (key_hdr->n_chunks != 4)){
		xprintf("����୮� ������⢮ ������ ������ ����� ������᭮�� (%d)\n",
				key_hdr->n_chunks);
		goto read_key_header_end;
	}else
		ret = true;
read_key_header_end:
	if (!usbkey_close(USB_KEY_NUMBER)){
		xprintf("�訡�� ������� ����� ������᭮��\n");
		ret = false;
	}
	if (ret)
		xprintf("ok\n");
	return ret;
}

/* �⥭�� ��⠫��� ������ � USB-���� */
static bool read_key_data(void)
{
	int remain = key_hdr->data_size - USB_BLOCK_LEN;
	if (remain > 0){
		div_t d = div(remain, USB_BLOCK_LEN);
		if (d.rem > 0)
			d.quot++;
		xprintf("�⥭�� ������ ����� ������᭮��: ");
		if (!usbkey_open(USB_KEY_NUMBER)){
			xprintf("�訡�� ������ ����� ������᭮��\n");
			return false;
		}else if (!usbkey_read(USB_KEY_NUMBER, key_buf + USB_BLOCK_LEN, 1, d.quot)){
			xprintf("�訡�� �⥭�� ������ ����� ������᭮��\n");
			usbkey_close(USB_KEY_NUMBER);
			return false;
		}else
			xprintf("ok\n");
		usbkey_close(USB_KEY_NUMBER);
	}
	return check_data();
}

/* �⥭�� ����� ������᭮�� � ᮧ����� ����室���� 䠩��� */
bool read_usbkey(void)
{
	bool ret = false;
	if (read_key_header()){
		usleep(500000);
		ret = read_key_data();
	}
	return ret;
}

/* �⥭�� 䠩�� �ਢ離� �� ����� ������᭮�� */
bool read_usbkey_bind(void)
{
	if (read_key_header()){
		key_hdr->n_chunks = 1;
		return check_data();
	}else
		return false;
}

/* �⥭�� �������⥫쭮� ���ଠ樨 �� ����� ������᭮�� */
bool read_usbkey_aux(struct key_aux_data *aux_data)
{
	bool ret = false;
	struct key_aux_data *p = (struct key_aux_data *)key_buf;
	xprintf("�⥭�� �������⥫��� ������ ����� ������᭮��: ");
	if (!usbkey_open(USB_KEY_NUMBER)){
		xprintf("�訡�� ������ ����� ������᭮��\n");
		return false;
	}else if (!usbkey_read(USB_KEY_NUMBER, key_buf, USB_KEY_BLOCKS - 1, 1)){
		xprintf("�訡�� �⥭�� ��������� ����� ������᭮��\n");
		goto read_usbkey_aux_end;
	}
	if (p->magic != USB_AUX_MAGIC){
		xprintf("�� �����㦥��\n");
		goto read_usbkey_aux_end;
	}
	memcpy(aux_data, p, sizeof(*p));
	ret = true;
read_usbkey_aux_end:
	if (!usbkey_close(USB_KEY_NUMBER)){
		xprintf("�訡�� ������� ����� ������᭮��\n");
		ret = false;
	}
	if (ret)
		xprintf("ok\n");
	return ret;
}
