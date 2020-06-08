/* Определение устройств, подключенных к терминалу. (c) gsr 2018-2020 */

#include <sys/ioctl.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "devinfo.h"
#include "genfunc.h"

/* Команды для опроса устройства */
#define CMD_ESC			0x1b
#define CMD_POLL		0x16
#define CMD_ENQ			0x7e
#define CMD_PARAMS		0x7d
#define CMD_ESC2		0x10
#define CMD_ETX			0x03

static struct dev_param *new_dev_param(const char *name, const char *val)
{
	assert(name != NULL);
	assert(val != NULL);
	struct dev_param *ret = malloc(sizeof(*ret));
	assert(ret != NULL);
	ret->next = NULL;
	ret->name = strdup(name);
	size_t len = strlen(val);
	bool quot = (len > 1) && (val[0] == '"') && (val[len - 1] == '"');
	if (quot){
		val++;
		len -= 2;
	}
	ret->val = malloc(len + 1);
	assert(ret->val != NULL);
	char *p = (char *)ret->val;
	memcpy(p, val, len);
	p[len] = 0;
	return ret;
}

static void free_dev_param(struct dev_param *param)
{
	if (param == NULL)
		return;
	if (param->name != NULL){
		free((char *)param->name);
		param->name = NULL;
	}
	if (param->val != NULL){
		free((char *)param->val);
		param->val = NULL;
	}
	free(param);
}

static struct dev_param_lst *new_dev_param_lst(void)
{
	struct dev_param_lst *ret = malloc(sizeof(*ret));
	assert(ret != NULL);
	ret->head = ret->tail = NULL;
	ret->nr_params = 0;
	return ret;
}

static void add_dev_param(struct dev_param_lst *lst, struct dev_param *param)
{
	assert(lst != NULL);
	assert(param != NULL);
	if (lst->head == NULL)
		lst->head = lst->tail = param;
	else{
		lst->tail->next = param;
		lst->tail = param;
	}
	param->next = NULL;
	lst->nr_params++;
}

static void free_param_lst(struct dev_param_lst *lst)
{
	if (lst == NULL)
		return;
	for (struct dev_param *p = lst->head, *pp = NULL; p != NULL; p = pp){
		pp = p->next;
		free_dev_param(p);
	}
	lst->head = lst->tail = NULL;
	lst->nr_params = 0;
}

static struct dev_info *new_dev_info(int type, const char *name)
{
	assert(name != NULL);
	struct dev_info *ret = malloc(sizeof(*ret));
	assert(ret != NULL);
	ret->next = NULL;
	ret->ttyS_name = NULL;
	ret->ss.csize = 8;
	ret->ss.parity = SERIAL_PARITY_NONE;
	ret->ss.stop_bits = SERIAL_STOPB_1;
	ret->ss.control = SERIAL_FLOW_NONE;
	ret->ss.baud = B0;
	ret->nr = 0;
	ret->type = type;
	ret->name = strdup(name);
	ret->params = NULL;
	return ret;
}

static void free_dev_info(struct dev_info *di)
{
	if (di == NULL)
		return;
	if (di->ttyS_name != NULL){
		free((char *)di->ttyS_name);
		di->ttyS_name = NULL;
	}
	if (di->name != NULL){
		free((char *)di->name);
		di->name = NULL;
	}
	if (di->params != NULL){
		free_param_lst(di->params);
		di->params = NULL;
	}
}

static struct dev_lst *new_dev_lst(void)
{
	struct dev_lst *ret = malloc(sizeof(*ret));
	assert(ret != NULL);
	ret->head = ret->tail = NULL;
	ret->nr_devs = 0;
	return ret;
}

void free_dev_lst(struct dev_lst *lst)
{
	if (lst == NULL)
		return;
	for (struct dev_info *p = lst->head, *pp = NULL; p != NULL; p = pp){
		pp = p->next;
		free_dev_info(p);
	}
	lst->head = lst->tail = NULL;
	lst->nr_devs = 0;
}

static void add_dev_info(struct dev_lst *lst, struct dev_info *di)
{
	assert(lst != NULL);
	assert(di != NULL);
	if (lst->head == NULL)
		lst->head = lst->tail = di;
	else{
		lst->tail->next = di;
		lst->tail = di;
	}
	di->next = NULL;
	lst->nr_devs++;
}

static void add_dev_lst(struct dev_lst *lst1, const struct dev_lst *lst2)
{
	assert(lst1 != NULL);
	assert(lst2 != NULL);
	if (lst2->nr_devs > 0){
		if (lst1->nr_devs == 0)
			*lst1 = *lst2;
		else{
			lst1->tail->next = lst2->head;
			lst1->tail = lst2->tail;
			lst1->nr_devs += lst2->nr_devs;
		}
	}
}

const struct dev_info *get_dev_info(const struct dev_lst *lst, int type)
{
	assert(lst != NULL);
	const struct dev_info *ret = NULL;
	for (const struct dev_info *di = lst->head; di != NULL; di = di->next){
		if (di->type == type){
			ret = di;
			break;
		}
	}
	return ret;
}

const char *get_dev_param_str(const struct dev_info *dev, const char *name)
{
	assert(dev != NULL);
	assert(name != NULL);
	const char *ret = NULL;
	for (const struct dev_param *p = dev->params->head; p != NULL; p = p->next){
		if (strcmp(p->name, name) == 0){
			ret = p->val;
			break;
		}
	}
	return ret;
}

uint32_t get_dev_param_uint(const struct dev_info *dev, const char *name)
{
	uint32_t ret = UINT32_MAX;
	const char *str = get_dev_param_str(dev, name);
	if (str != NULL){
		char *p = NULL;
		ret = strtoul(str, &p, 10);
		if (*p != 0)
			ret = UINT32_MAX;
	}
	return ret;
}

uint32_t get_dev_param_ip(const struct dev_info *dev, const char *name)
{
	uint32_t ret = 0;
	const char *str = get_dev_param_str(dev, name);
	if ((str == NULL) || (strlen(str) != 15))
		return 0;
	uint32_t v[4] = {[0 ... ASIZE(v) - 1] = 0};
	bool err = false;
	inline bool is_digit(char c)
	{
		return (c >= '0') && (c <= '9');
	}
	for (int i = 0, k = 0; i < 15; i++){
		char c = str[i];
		if ((i == 3) || (i == 7) || (i == 11)){
			if (c == '.')
				k++;
			else
				err = true;
		}else if (is_digit(c)){
			v[k] *= 10;
			v[k] += c - '0';
		}else
			err = true;
		if (err)
			break;
	}
	if (!err)
		err = (v[0] > 255) || (v[1] > 255) || (v[2] > 255) || (v[3] > 255);
	if (!err)
		ret = (v[3] << 24) | (v[2] << 16) | (v[1] << 8) | v[0];
	return ret;
}

/* Размер буфера приёма */
#define RX_BUF_SIZE		4096
/* Буфер приёма */
static uint8_t rx[RX_BUF_SIZE];
/* Длина полученных данных */
static size_t rx_len = 0;
/* Индекс следующего байта для чтения */
static off_t rx_idx = 0;
/* Сброс буфера приёма */
static void reset_rx(void)
{
	rx_len = rx_idx = 0;
}

static bool send_poll(int dev, uint32_t *timeout)
{
	assert(dev != -1);
	assert(timeout != NULL);
	bool ret = false;
	static uint8_t cmd[] = {CMD_ESC, CMD_POLL, CMD_PARAMS, 0x00};
	const char *dev_name = fd2name(dev);
	ssize_t rc = serial_write(dev, cmd, sizeof(cmd), timeout);
	if (rc == -1)
		fprintf(stderr, "%s: ошибка записи в %s: %s.\n", __func__,
			dev_name, strerror(errno));
	else if (rc == sizeof(cmd))
		ret = true;
	else
		fprintf(stderr, "%s: в %s записано %zd байт вместо %zu.\n", __func__,
			dev_name, rc, sizeof(cmd));
	return ret;
}

static bool read_status(int dev, uint8_t *status, uint32_t *timeout)
{
	assert(dev != -1);
	assert(status != NULL);
	assert(timeout != NULL);
	*status = 0xff;
	if ((rx_len + 3) > sizeof(rx)){
		fprintf(stderr, "%s: переполнение буфера чтения.\n", __func__);
		return false;
	}
	const char *dev_name = fd2name(dev);
	bool ret = false;
	ssize_t rc = serial_read(dev, rx + rx_len, 3, timeout);
	if (rc == -1)
		fprintf(stderr, "%s: ошибка чтения из %s: %s.\n", __func__,
			dev_name, strerror(errno));
	else if (rc != 3)
		fprintf(stderr, "%s: из %s прочитано %zd байт вместо 3.\n",
			__func__, dev_name, rc);
	else if ((rx[rx_len] != CMD_ESC2) || (rx[rx_len + 1] != CMD_POLL))
		fprintf(stderr, "%s: неверный формат ответа из %s.\n",
			__func__, dev_name);
	else{
		*status = rx[rx_len + 2];
		rx_len += rc;
		ret = true;
	}
	return ret;
}

static bool read_data(int dev, uint32_t *timeout)
{
	if (rx_len >= sizeof(rx)){
		fprintf(stderr, "%s: переполнение буфера чтения.\n", __func__);
		return false;
	}
	const char *dev_name = fd2name(dev);
	ssize_t rc = serial_read(dev, rx + rx_len, 1, timeout);
	if (rc == -1){
		fprintf(stderr, "%s: ошибка чтения из %s: %s.\n", __func__,
			dev_name, strerror(errno));
		return false;
	}else if (rc != 1){
		fprintf(stderr, "%s: из %s прочитано %zd байт вместо 1.\n",
			__func__, dev_name, rc);
		return false;
	}
	size_t nr_devs = rx[rx_len++];
	if ((nr_devs == 0) || (nr_devs > MAX_SUBDEVICES)){
		fprintf(stderr, "%s: неверное количество устройств "
			"в составном устройстве: %zu.\n", __func__, nr_devs);
		return false;
	}
	uint32_t t0 = u_times(), dt = 0;
	size_t n = 0;
	while ((n < nr_devs) && (rx_len < sizeof(rx)) && (dt < *timeout)){
		ssize_t rc = read(dev, rx + rx_len, sizeof(rx) - rx_len);
		if ((rc == -1) && (errno != EWOULDBLOCK)){
			fprintf(stderr, "%s: ошибка чтения из %s: %s.\n",
				__func__, dev_name, strerror(errno));
			break;
		}else if (rc > 0){
			for (int i = 0; (i < rc) && (n < nr_devs); i++){
				if (rx[rx_len++] == CMD_ETX)
					n++;
			}
		}else
			dt = u_times() - t0;
	}
	if (dt < *timeout)
		*timeout -= dt;
	else
		*timeout = 0;
	return n == nr_devs;
}

static size_t nr_devs = 0;
static char dev_type_str[MAX_DEV_TYPE_LEN + 1];
static int dev_type = DEV_UNKNOWN;
static char dev_name[MAX_DEV_NAME_LEN + 1];
static char dev_param_name[MAX_DEV_PARAM_NAME_LEN + 1];
static char dev_param_val[MAX_DEV_PARAM_VAL_LEN + 1];
static int idx = 0;

static struct dev_info *di = NULL;

static enum {
	st_dev_type,
	st_skip_dev,
	st_dev_name,
	st_param_name,
	st_param_val,
	st_dev_end,
	st_err,
	st_end,
} st = st_dev_name;

static int get_dev_type(const char *type)
{
	assert(type != NULL);
	int ret = DEV_UNKNOWN;
	if (strcmp(type, "PRINTER") == 0)
		ret = DEV_XPRN;
	else if (strcmp(type, "RFID") == 0)
		ret = DEV_RFID;
	else if (strcmp(type, "PAYMENT") == 0)
		ret = DEV_POS;
	else if (strcmp(type, "BARCODE") == 0)
		ret = DEV_SCANNER;
	else if (strcmp(type, "FISCALPRINTER") == 0)
		ret = DEV_KKT;
	return ret;
}

static void on_st_dev_type(uint8_t b)
{
	if (b == '='){
		dev_type = get_dev_type(dev_type_str);
		if (dev_type == DEV_UNKNOWN)
			st = st_skip_dev;
		else{
			idx = 0;
			st = st_dev_name;
		}
	}else if ((b > 0x20) && (b < 0x7f) && (idx < MAX_DEV_TYPE_LEN))
		dev_type_str[idx++] = b;
	else
		st = st_err;
}

static void on_st_skip_dev(uint8_t b)
{
	if (b == CMD_ETX){
		if (nr_devs > 0){
			nr_devs--;
			if (nr_devs == 0)
				st = st_end;
			else{
				idx = 0;
				st = st_dev_type;
			}
		}else
			st = st_end;
	}
}

static void on_st_dev_name(uint8_t b)
{
	if ((b == ';') || (b == CMD_ETX)){
		dev_name[idx] = 0;
		di = new_dev_info(dev_type, dev_name);
		if (b == ';'){
			idx = 0;
			st = st_param_name;
		}else
			st = st_dev_end;
	}else if ((b > 0x20) && (b < 0x7f) && (idx < MAX_DEV_NAME_LEN))
		dev_name[idx++] = recode(b);
	else
		st = st_err;
}

static void on_st_param_name(uint8_t b)
{
	if (b == '='){
		dev_param_name[idx] = 0;
		idx = 0;
		st = st_param_val;
	}else if ((b > 0x20) && (b < 0x7f) && (idx < MAX_DEV_PARAM_NAME_LEN))
		dev_param_name[idx++] = b;
	else
		st = st_err;
}

static void on_st_param_val(uint8_t b)
{
	if ((b == ';') || (b == CMD_ETX)){
		dev_param_val[idx] = 0;
		struct dev_param *param = new_dev_param(dev_param_name, dev_param_val);
		if ((di == NULL) || (param == NULL))
			st = st_err;
		else{
			if (di->params == NULL)
				di->params = new_dev_param_lst();
			if (di->params == NULL)
				st = st_err;
			else{
				add_dev_param(di->params, param);
				if (b == CMD_ETX)
					st = st_dev_end;
				else{
					idx = 0;
					st = st_param_name;
				}
			}
		}
	}else if ((b >= 0x20) && (idx < MAX_DEV_PARAM_VAL_LEN))
		dev_param_val[idx++] = recode(b);
	else
		st = st_err;
}

static struct dev_lst *parse_data(const uint8_t *data, size_t len,
		const char *ttyS_name, const struct serial_settings *ss)
{
	assert(data != NULL);
	assert(len > 0);
	assert(ttyS_name != NULL);
	assert(ss != NULL);
	struct dev_lst *devs = NULL;
	nr_devs = data[0];	/* правильность значения была проверена ранее */
	di = NULL;
	idx = 0;
	st = st_dev_type;
	for (int i = 1, nr = 1; i < len; i++){
		uint8_t b = data[i];
		switch (st){
			case st_dev_type:
				on_st_dev_type(b);
				break;
			case st_skip_dev:
				on_st_skip_dev(b);
				break;
			case st_dev_name:
				on_st_dev_name(b);
				break;
			case st_param_name:
				on_st_param_name(b);
				break;
			case st_param_val:
				on_st_param_val(b);
				break;
		}
		if (st == st_dev_end){
			di->ttyS_name = strdup(ttyS_name);
			di->ss = *ss;
			di->nr = nr++;
			if (devs == NULL)
				devs = new_dev_lst();
			add_dev_info(devs, di);
			di = NULL;
			nr_devs--;
			if (nr_devs == 0)
				st = st_end;
			else{
				idx = 0;
				st = st_dev_type;
			}
		}else if (st == st_err){
			free_dev_lst(devs);
			devs = NULL;
		}
		if (st == st_end)
			break;
	}
	return devs;
}

static struct dev_lst *poll_vcom(const char *name)
{
	static struct serial_settings ss[] = {
		{CS8, SERIAL_PARITY_NONE, SERIAL_STOPB_1, SERIAL_FLOW_NONE, B115200},
		{CS8, SERIAL_PARITY_NONE, SERIAL_STOPB_1, SERIAL_FLOW_RTSCTS, B115200},
	};
	struct dev_lst *devs = NULL;
	for (int i = 0; i < ASIZE(ss); i++){
		int dev = serial_open(name, ss + i, O_RDWR);
		if (dev == -1)
			continue;
		else if (ss[i].control == SERIAL_FLOW_RTSCTS)
			usleep(100000);		/* 100 мс */
		else
			serial_ch_lines(dev, TIOCM_RTS, false);
		serial_flush(dev, TCIOFLUSH);
		uint32_t timeout = DEV_INFO_TIMEOUT;
		if (send_poll(dev, &timeout)){
			reset_rx();
			uint8_t status;
			if (read_status(dev, &status, &timeout) && (status == 0) &&
					read_data(dev, &timeout) && (rx_len > 3))
				devs = parse_data(rx + 3, rx_len - 3, name, ss + i);
		}
		serial_close(dev);
		if (devs != NULL)
			break;
	}
	return devs;
}

struct dev_lst *poll_devices(void)
{
#define USB_SERIAL_DIR		"/sys/bus/usb-serial/devices"
#define DEV_DIR			"/dev"
	int selector(const struct dirent *entry)
	{
		uint32_t n = 0;
		char dummy = 0;
		return sscanf(entry->d_name, "ttyUSB%u%c", &n, &dummy) == 1;
	}
	struct dirent **names;
	int n = scandir(USB_SERIAL_DIR, &names, selector, alphasort);
	if (n == -1){
		fprintf(stderr, "%s: ошибка просмотра каталога " USB_SERIAL_DIR ": %s\n",
			__func__, strerror(errno));
		return NULL;
	}
	struct dev_lst *devs = NULL;
	for (int i = 0; i < n; i++){
		static char path[128];
		snprintf(path, sizeof(path), DEV_DIR "/%s", names[i]->d_name);
		struct dev_lst *lst = poll_vcom(path);
		if (lst != NULL){
			if (devs == NULL)
				devs = lst;
			else
				add_dev_lst(devs, lst);
		}
	}
	free(names);
	return devs;
}
