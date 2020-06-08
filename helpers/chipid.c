/* Определение типа адаптера BSC. (c) gsr 2004 */

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "sys/chipset.h"
#include "sys/ioctls.h"

int get_chipset_type(void)
{
	int dev = open(CHIPSET_DEV_NAME, O_RDONLY), ret;
	if (dev == -1){
		fprintf(stderr, "Ошибка открытия " CHIPSET_DEV_NAME
			" для чтения: %s.\n", strerror(errno));
		return chipsetUNKNOWN;
	}
	ret = ioctl(dev, CHIPSET_IO_CHIP_TYPE);
	close(dev);
	return (ret == -1) ? chipsetUNKNOWN : ret;
}

int main()
{
	return get_chipset_type();
}
