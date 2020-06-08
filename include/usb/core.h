#ifndef USB_CORE_H
#define USB_CORE_H

#include "sysdefs.h"

/* ���ᨬ��쭮� �᫮ ���㫥� ������᭮�� */
#define NR_USB_KEYS		16

/* ������ ������ USB-���� */
#define USB_KEY_SIZE		65536
/* ������ ����� �� ࠡ�� � USB-���箬 */
#define USB_BLOCK_LEN		256
/* ������ ������ � ������ */
#define USB_KEY_BLOCKS		(USB_KEY_SIZE / USB_BLOCK_LEN)

/* ����⨥ ����� ������᭮�� � ������� ����஬ */
extern bool usbkey_open(int n);
/* �����⨥ ����� ������᭮�� � ������� ����஬ */
extern bool usbkey_close(int n);
/*
 * �⥭�� ������ � USB-����. ������ � ᬥ饭�� ������ 㪠�뢠���� �
 * USB_BLOCK_LEN-������ ������.
 */
extern bool usbkey_read(int n, uint8_t *data, int start, int count);
/* ������ ������ � ����� ������᭮�� � ������� ����஬ */
extern bool usbkey_write(int n, uint8_t *data);

#endif /* USB_CORE_H */

