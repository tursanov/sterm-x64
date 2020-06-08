/* Определение устройств, подключенных к терминалу. (c) gsr 2018-2019 */

#if !defined DEVINFO_H
#define DEVINFO_H

#include "serial.h"

/* Тиы устройств */
enum {
	DEV_UNKNOWN = -1,	/* неизвестное устройство */
	DEV_XPRN,		/* БПУ */
	DEV_RFID,		/* считыватель ЭМТТ */
	DEV_POS,		/* ИПТ */
	DEV_SCANNER,		/* сканер */
	DEV_KKT,		/* ККТ */
};

/* Максимальное количество устройств в составном устройстве */
#define MAX_SUBDEVICES		3

/* Параметр устройства */
struct dev_param {
	struct dev_param *next;	/* указатель на следующий элемент списка */
#define MAX_DEV_PARAM_NAME_LEN	32
	const char *name;	/* название параметра */
#define MAX_DEV_PARAM_VAL_LEN	64
	const char *val;	/* значение параметра */
};

struct dev_param_lst {
	struct dev_param *head;
	struct dev_param *tail;
	size_t nr_params;
};

/* Информация об устройстве */
struct dev_info {
	struct dev_info *next;		/* указатель на следующий экземпляр структуры */
	const char *ttyS_name;		/* /dev/ttyS для работы с устройством */
	struct serial_settings ss;	/* параметры работы виртуального COM-порта */
	uint32_t nr;			/* номер устройства в составе многофункционального устройства */
#define MAX_DEV_TYPE_LEN	32
	int type;			/* тип устройства */
#define MAX_DEV_NAME_LEN	64
	const char *name;		/* название устройства */
	struct dev_param_lst *params;	/* параметры устройства */
};

struct dev_lst {
	struct dev_info *head;
	struct dev_info *tail;
	size_t nr_devs;
};

extern const struct dev_info *get_dev_info(const struct dev_lst *lst, int type);
extern void free_dev_lst(struct dev_lst *lst);

/* Таймаут получения информации об устройстве */
#define DEV_INFO_TIMEOUT	100	/* 1 сек */

extern struct dev_lst *poll_devices(void);

/* Некоторые предопределённые параметры */
/* ККТ */
#define KKT_VERSION		"VERSION"	/* версия ПО ККТ */
#define KKT_SN			"SN-FISCAL"	/* заводской номер ККТ */
#define KKT_FS_SN		"SN-FN"		/* заводской номер ФН */

#define KKT_FLOW_CTL		"FLOW_CONTROL"	/* тип управления потоком виртуального COM-порта */
#define KKT_FLOW_CTL_NONE	0
#define KKT_FLOW_CTL_XON_XOFF	1
#define KKT_FLOW_CTL_RTS_CTS	2
#define KKT_FLOW_CTL_DTR_DSR	3

#define KKT_ETH_IP		"IP"		/* ip-адрес ККТ */
#define KKT_ETH_NETMASK		"NETMASK"	/* маска подсети ККТ */
#define KKT_ETH_GW		"GW"		/* ip-адрес шлюза локальной сети */

#define KKT_GPRS_APN		"GPRS_APN"	/* имя точки доступа GPRS */
#define KKT_GPRS_USER		"GPRS_USER"	/* имя пользователя GPRS */
#define KKT_GPRS_PASSWORD	"GPRS_PASSWORD"	/* пароль пользователя GPRS */

#define KKT_FDO_IFACE		"INTERFACE_KKT_OFD"	/* тип интерфейса с ОФД */
#define KKT_FDO_IFACE_USB	0
#define KKT_FDO_IFACE_ETH	1
#define KKT_FDO_IFACE_GPRS	2
#define KKT_FDO_IFACE_INT	3

#define KKT_FDO_IP		"IP_ADDRESS"	/* ip-адрес ОФД */
#define KKT_FDO_PORT		"TCP_PORT"	/* TCP-порт ОФД */

extern const char *get_dev_param_str(const struct dev_info *dev, const char *name);
extern uint32_t get_dev_param_uint(const struct dev_info *dev, const char *name);
extern uint32_t get_dev_param_ip(const struct dev_info *dev, const char *name);

#endif		/* DEVINFO_H */
