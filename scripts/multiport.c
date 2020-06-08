/* Работа с мультипортовыми картами для COM-портов. (c) gsr 2005 */

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <linux/serial.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sysdefs.h"

#define PCI_BUS_DIR	"/proc/bus/pci/"
#define PCI_DEVICES	PCI_BUS_DIR "devices"

#define PCI_REGION_FLAG_MASK 0x0fU	/* перенесено из linux/pci.h (__KERNEL__) */

struct multiport_desc {
	int irq;
	uint32_t io_base[6];
};

bool look_dev_info(uint16_t vendor, uint16_t device, struct multiport_desc *desc)
{
	char buf[256], *p;
	FILE *f = fopen(PCI_DEVICES, "r");
	uint8_t bus_number, dev_fn, irq;
	uint16_t vend, dev;
	uint32_t iobase[7], iolen[7];
	bool nl;
	int n, c, i;
	if (f == NULL){
		fprintf(stderr, "Ошибка открытия " PCI_DEVICES " для чтения: %s.\n",
			strerror(errno));
		return false;
	}
	while (fgets(buf, sizeof(buf), f) != NULL){
		nl = false;
		for (p = buf; *p; p++){
			if (*p == '\n')
				nl = true;
			else
				*p = toupper(*p);
		}
		n = sscanf(buf, "%2hhX%2hhX %4hX%4hX %hhX"
				" %8X %8X %8X %8X %8X %8X %8X"
				" %8X %8X %8X %8X %8X %8X %8X",
			&bus_number, &dev_fn, &vend, &dev, &irq,
			iobase + 0, iobase + 1, iobase + 2, iobase + 3,
			iobase + 4, iobase + 5, iobase + 6,
			iolen + 0, iolen + 1, iolen + 2, iolen + 3,
			iolen + 4, iolen + 5, iolen + 6);
		if (!nl)
			while (((c = fgetc(f)) != '\n') && (c != EOF));
		if ((n == 19) && (vend == vendor) && (dev == device)){
			desc->irq = irq;
			for (i = 0; i < ASIZE(desc->io_base); i++){
				if (iolen[i] != 0)
					desc->io_base[i] = iobase[i] & ~PCI_REGION_FLAG_MASK;
			}
			return irq != 0;
		}
	}
	return false;
}

bool find_multiport(struct multiport_desc *desc)
{
	int is_dir(const struct dirent *entry)
	{
		char name[128];
		struct stat st;
		snprintf(name, sizeof(name), PCI_BUS_DIR "%s", entry->d_name);
		if (stat(name, &st) == -1)
			return 0;
		else
			return S_ISDIR(st.st_mode) &&
				strcmp(entry->d_name, ".") &&
				strcmp(entry->d_name, "..");
	}
	struct dirent **dirs, **files;
	int i, n, m, cur_dir, fd;
	int is_reg(const struct dirent *entry)
	{
		char name[128];
		struct stat st;
		snprintf(name, sizeof(name), PCI_BUS_DIR "%s/%s",
				dirs[cur_dir]->d_name, entry->d_name);
		if (stat(name, &st) == -1)
			return 0;
		else
			return S_ISREG(st.st_mode) && (st.st_size == 256);
	}
	void free_direntries(struct dirent **p, int n)
	{
		int i;
		for (i = 0; i < n; i++)
			free(p[i]);
		free(p);
	}
	bool flag = false;
	desc->irq = 0;
	n = scandir(PCI_BUS_DIR, &dirs, is_dir, alphasort);
	if (n == -1){
		fprintf(stderr, "Ошибка просмотра " PCI_BUS_DIR ": %s.\n",
			strerror(errno));
		return false;
	}
	for (cur_dir = 0; !flag && (desc->irq == 0) && (cur_dir < n); cur_dir++){
		char name[128];
		uint8_t buf[256];
		uint16_t cls;
		snprintf(name, sizeof(name), PCI_BUS_DIR "%s",
			dirs[cur_dir]->d_name);
		m = scandir(name, &files, is_reg, alphasort);
		if (m == -1){
			fprintf(stderr, "Ошибка сканирования %s: %s.\n",
				name, strerror(errno));
			continue;
		}
		for (i = 0; !flag && (desc->irq == 0) && (i < m); i++){
			snprintf(name, sizeof(name), PCI_BUS_DIR "%s/%s",
				dirs[cur_dir]->d_name, files[i]->d_name);
			fd = open(name, O_RDONLY);
			if (fd == -1){
				fprintf(stderr, "Ошибка открытия %s для чтения: %s.\n",
					name, strerror(errno));
				continue;
			}
			if (read(fd, buf, sizeof(buf)) != sizeof(buf)){
				fprintf(stderr, "Ошибка чтения из %s: %s.\n",
					name, strerror(errno));
				close(fd);
				continue;
			}
			cls = *(uint16_t *)(buf + PCI_CLASS_DEVICE);
			if ((cls == PCI_CLASS_COMMUNICATION_SERIAL) ||
					(cls == PCI_CLASS_COMMUNICATION_MULTISERIAL) ||
					(cls == PCI_CLASS_COMMUNICATION_OTHER)){
				flag = look_dev_info(*(uint16_t *)(buf + PCI_VENDOR_ID),
					*(uint16_t *)(buf + PCI_DEVICE_ID), desc);
			}
			close(fd);
		}
		free_direntries(files, m);
	}
	free_direntries(dirs, n);
	return desc->irq != 0;
}

void set_serial(struct multiport_desc *desc)
{
#define NR_PORTS	8
	struct serial_struct ss;
	int fd, i, k;
	char name[16];
/* Находим первый несконфигурированный порт */
	for (i = 0; i < NR_PORTS; i++){
		snprintf(name, sizeof(name), "/dev/ttyS%d", i);
		fd = open(name, O_RDONLY);
		k = (fd != -1) && (ioctl(fd, TIOCGSERIAL, &ss) != -1);
		if (fd != -1)
			close(fd);
		if (k){
			if (ss.type == PORT_UNKNOWN)
				break;
		}else
			return;
	}
/* Конфигурируем ненастроенные порты */
	for (k = i; (i < NR_PORTS) && (desc->io_base[i - k] != 0); i++){
		snprintf(name, sizeof(name), "/dev/ttyS%d", i);
		fd = open(name, O_RDONLY);
		if (fd == -1){
			fprintf(stderr, "Ошибка открытия %s для чтения: %s.\n",
				name, strerror(errno));
			continue;
		}
		if (ioctl(fd, TIOCGSERIAL, &ss) == -1){
			fprintf(stderr, "Ошибка TIOCGSERIAL для %s: %s.\n",
				name, strerror(errno));
			close(fd);
			continue;
		}
		ss.type = PORT_16550A;
		ss.port = desc->io_base[i - k];
		ss.irq = desc->irq;
		ss.flags |= ASYNC_SKIP_TEST;
		ss.baud_base = 115200;
		if (ioctl(fd, TIOCSSERIAL, &ss) == -1){
			fprintf(stderr, "Ошибка TIOCSSERIAL для %s: %s.\n",
				name, strerror(errno));
			close(fd);
			continue;
		}
		close(fd);
		printf("COM%d:\ti/o: 0x%.8x; irq = %d\n",
			i, desc->io_base[i - k], desc->irq);
	}
}

int main()
{
	struct multiport_desc desc;
	if (find_multiport(&desc))
		set_serial(&desc);
	return 0;
}
