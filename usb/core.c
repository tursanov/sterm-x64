/* Базовые функции для работы с модулями безопасности. (c) gsr & Alex 2004 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sys/ioctls.h"
#include "usb/core.h"

/* Дескрипторы модулей безопасности */
static int fds[NR_USB_KEYS] = {[0 ... NR_USB_KEYS - 1] = -1};

/* Открытие модуля безопасности с заданным номером */
bool usbkey_open(int n)
{
	static char name[16];
	if ((n < 0) || (n >= NR_USB_KEYS))
		return false;
	snprintf(name, sizeof(name), USBKEY_BASE_NAME "%d", n);
	fds[n] = open(name, O_RDWR);
/*	if (fds[n] == -1)
		fprintf(stderr, "Ошибка открытия %s для чтения/записи: %s.\n",
			name, strerror(errno));*/
	return fds[n] != -1;
}

/* Закрытие модуля безопасности с заданным номером */
bool usbkey_close(int n)
{
	if ((n < 0) || (n >= NR_USB_KEYS))
		return false;
	if (fds[n] != -1){
		close(fds[n]);
		fds[n] = -1;
	}
	return true;
}

/* Чтение данных из модуля безопасности с заданным номером */
bool usbkey_read(int n, uint8_t *data, int start, int count)
{
	if ((n < 0) || (n >= NR_USB_KEYS) || (fds[n] == -1) ||
			(data == NULL))
		return false;
	lseek(fds[n], start * USB_BLOCK_LEN, SEEK_SET);
	return read(fds[n], data, count * USB_BLOCK_LEN) != -1;
}

/* Запись данных в модуль безопасности с заданным номером */
bool usbkey_write(int n, uint8_t *data)
{
	if ((n < 0) || (n >= NR_USB_KEYS) || (fds[n] == -1) || (data == NULL))
		return false;
	else
		return write(fds[n], data, USB_KEY_SIZE) != -1;
}
