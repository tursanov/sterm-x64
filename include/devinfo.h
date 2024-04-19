/* ��।������ ���ன��, ������祭��� � �ନ����. (c) gsr 2018-2019 */

#if !defined DEVINFO_H
#define DEVINFO_H

#if defined __cplusplus
extern "C" {
#endif

#include "serial.h"

/* ��� ���ன�� */
enum {
	DEV_UNKNOWN = -1,	/* �������⭮� ���ன�⢮ */
	DEV_XPRN,		/* ��� */
	DEV_RFID,		/* ���뢠⥫� ���� */
	DEV_POS,		/* ��� */
	DEV_SCANNER,		/* ᪠��� */
	DEV_KKT,		/* ��� */
};

/* ���ᨬ��쭮� ������⢮ ���ன�� � ��⠢��� ���ன�⢥ */
#define MAX_SUBDEVICES		3

/* ��ࠬ��� ���ன�⢠ */
struct dev_param {
	struct dev_param *next;	/* 㪠��⥫� �� ᫥���騩 ����� ᯨ᪠ */
#define MAX_DEV_PARAM_NAME_LEN	32
	const char *name;	/* �������� ��ࠬ��� */
#define MAX_DEV_PARAM_VAL_LEN	64
	const char *val;	/* ���祭�� ��ࠬ��� */
};

struct dev_param_lst {
	struct dev_param *head;
	struct dev_param *tail;
	size_t nr_params;
};

/* ���ଠ�� �� ���ன�⢥ */
struct dev_info {
	struct dev_info *next;		/* 㪠��⥫� �� ᫥���騩 ������� �������� */
	const char *ttyS_name;		/* /dev/ttyS ��� ࠡ��� � ���ன�⢮� */
	struct serial_settings ss;	/* ��ࠬ���� ࠡ��� ����㠫쭮�� COM-���� */
	uint32_t nr;			/* ����� ���ன�⢠ � ��⠢� ������㭪樮���쭮�� ���ன�⢠ */
#define MAX_DEV_TYPE_LEN	32
	int type;			/* ⨯ ���ன�⢠ */
#define MAX_DEV_NAME_LEN	64
	const char *name;		/* �������� ���ன�⢠ */
	struct dev_param_lst *params;	/* ��ࠬ���� ���ன�⢠ */
};

struct dev_lst {
	struct dev_info *head;
	struct dev_info *tail;
	size_t nr_devs;
};

extern const struct dev_info *get_dev_info(const struct dev_lst *lst, int type);
extern void free_dev_lst(struct dev_lst *lst);

/* ������� ����祭�� ���ଠ樨 �� ���ன�⢥ */
#define DEV_INFO_TIMEOUT	100	/* 1 ᥪ */

extern struct dev_lst *poll_devices(void);

/* ������� �।��।���� ��ࠬ���� */
/* ��� */
#define KKT_VERSION		"VERSION"	/* ����� �� ��� */
#define KKT_SN			"SN-FISCAL"	/* �����᪮� ����� ��� */
#define KKT_FS_SN		"SN-FN"		/* �����᪮� ����� �� */

#define KKT_FLOW_CTL		"FLOW_CONTROL"	/* ⨯ �ࠢ����� ��⮪�� ����㠫쭮�� COM-���� */
#define KKT_FLOW_CTL_NONE	0
#define KKT_FLOW_CTL_XON_XOFF	1
#define KKT_FLOW_CTL_RTS_CTS	2
#define KKT_FLOW_CTL_DTR_DSR	3

#define KKT_ETH_IP		"IP"		/* ip-���� ��� */
#define KKT_ETH_NETMASK		"NETMASK"	/* ��᪠ ����� ��� */
#define KKT_ETH_GW		"GW"		/* ip-���� � �����쭮� �� */

#define KKT_GPRS_APN		"GPRS_APN"	/* ��� �窨 ����㯠 GPRS */
#define KKT_GPRS_USER		"GPRS_USER"	/* ��� ���짮��⥫� GPRS */
#define KKT_GPRS_PASSWORD	"GPRS_PASSWORD"	/* ��஫� ���짮��⥫� GPRS */

#define KKT_FDO_IFACE		"INTERFACE_KKT_OFD"	/* ⨯ ����䥩� � ��� */
#define KKT_FDO_IFACE_USB	0
#define KKT_FDO_IFACE_ETH	1
#define KKT_FDO_IFACE_GPRS	2
#define KKT_FDO_IFACE_INT	3

#define KKT_FDO_IP		"IP_ADDRESS"	/* ip-���� ��� */
#define KKT_FDO_PORT		"TCP_PORT"	/* TCP-���� ��� */

extern const char *get_dev_param_str(const struct dev_info *dev, const char *name);
extern uint32_t get_dev_param_uint(const struct dev_info *dev, const char *name);
extern uint32_t get_dev_param_ip(const struct dev_info *dev, const char *name);

#if defined __cplusplus
}
#endif

#endif		/* DEVINFO_H */
