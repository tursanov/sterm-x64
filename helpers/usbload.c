#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "usb/core.h"
#include "usb/key.h"
#include "md5.h"

void usage(void)
{
	printf("�ᯮ�짮�����: usbload <key-file>\n");
}

/* ���� ��� ����� � ����� ������᭮�� */
static uint8_t key_buf[USB_KEY_SIZE];
static struct key_header *key_hdr;
static struct key_data_header *data_hdr;
static uint8_t *key_data;
/* ���� ��� �⥭�� ����� ������᭮�� */
static uint8_t key_rx_buf[USB_KEY_SIZE];

/* ������ ��������� 䠩�� � ����� ������᭮�� */
static bool write_file_to_buf(char *name)
{
	struct stat st;
	int l, fd;
	bool flag = false;
	if (name == NULL)
		return false;
/* ��⠥� ����� */
	if (stat(name, &st) == -1){
		fprintf(stderr, "�訡�� ����祭�� ���ଠ樨 � %s: %s.\n",
			name, strerror(errno));
		return false;
	}
	l = USB_KEY_SIZE - (sizeof(*key_hdr) + sizeof(*data_hdr));
	if (st.st_size > l){
		fprintf(stderr, "ࠧ��� 䠩�� �� ����� �ॢ���� %d ����\n", l);
		return false;
	}
	fd = open(name, O_RDONLY);
	if (fd == -1){
		fprintf(stderr, "�訡�� ������ %s ��� �⥭��: %s.\n",
			name, strerror(errno));
		return false;
	}
	if (read(fd, key_data, st.st_size) != st.st_size){
		fprintf(stderr, "�訡�� �⥭�� �� %s: %s.\n",
			name, strerror(errno));
		close(fd);
		return false;
	}
	close(fd);
/* ��⠭�������� ����室��� ��������� */
	key_hdr->magic = USB_KEY_MAGIC;
	key_hdr->data_size = st.st_size + sizeof(*key_hdr) + sizeof(*data_hdr);
	key_hdr->n_chunks = 1;
	data_hdr->len = st.st_size + sizeof(*data_hdr);
	get_md5(key_data, st.st_size, &data_hdr->crc);
/* �����뢠�� � ����� */
	if (!usbkey_open(0)){
		fprintf(stderr, "�訡�� ���樠����樨 ����� ������᭮��\n");
		return false;
	}
	printf("������ ������ � ����� ������᭮��...");
	fflush(stdout);
/* �᫨ ������ ��諠 �ᯥ譮, �஢��塞 ����ᠭ�� ����� */
/*	usbkey_write(0, key_buf);*/
	if (usbkey_write(0, key_buf)){
		printf("��\n�஢�ઠ ����ᠭ��� ������...");
		fflush(stdout);
		if (usbkey_read(0, key_rx_buf, 0, USB_KEY_BLOCKS)){
			flag = memcmp(key_buf, key_rx_buf, sizeof(key_buf)) == 0;
			printf("%s\n", flag ? "��" : "��ᮢ������� ������");
		}else
			printf("�訡�� �⥭��\n");
	}else
		printf("�訡��\n");
	if (!usbkey_close(0)){
		fprintf(stderr, "�訡�� ������� ����� ������᭮��\n");
		return false;
	}
	return flag;
}

int main(int argc, char *argv[])
{
	if (argc != 2){
		usage();
		return 1; 
	}
	key_hdr = (struct key_header *)key_buf;
	data_hdr = (struct key_data_header *)(key_hdr + 1);
	key_data = (uint8_t *)(data_hdr + 1);
	if (write_file_to_buf(argv[1]))
		return 0;
	else
		return 1;
}
