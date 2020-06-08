/* ������� ��� ࠡ��� � ���. (c) gsr 2018-2019 */

#if !defined KKT_CMD_H
#define KKT_CMD_H

#include "sysdefs.h"

/* �������⮢� ������� */
#define KKT_NUL			0x00		/* ����� ������� */
#define KKT_STX			0x02		/* ��砫� ⥪�� �� */
#define KKT_ETX			0x03		/* ����� ⥪�� �� */
#define KKT_ALIGN_LEFT		0x04		/* ��ࠢ������� �� ������ ��� */
#define KKT_ALIGN_RIGHT		0x05		/* ��ࠢ������� �� �ࠢ��� ��� */
#define KKT_ALIGN_CENTER	0x06		/* ��ࠢ������� �� 業��� */
#define KKT_WIDE_FNT		0x07		/* �ப�� ���� */
#define KKT_BS			0x08		/* 蠣 ����� */
#define KKT_ITALIC		0x09		/* �������� ���� */
#define KKT_LF			0x0a		/* ��ॢ�� ��ப� */
#define KKT_NORM_FNT		0x0b		/* ��ଠ��� ���� */
#define KKT_FF			0x0c		/* ��१�� � ���ᮬ ������ */
#define KKT_CR			0x0d		/* ������ ���⪨ */
#define KKT_NR			0x0e		/* ���浪��� ����� */
#define KKT_ESC2		0x10		/* ��砫� �⢥� �� �� */
#define KKT_UNBREAK_ON		0x11		/* ��砫� ��ࠧ�뢭��� ⥪�� */
#define KKT_UNBREAK_OFF		0x12		/* ����� ��ࠧ�뢭��� ⥪�� */
#define KKT_UNDERLINE		0x14		/* �����ભ��� ���� */
#define KKT_BELL		0x16		/* ��㪮��� ᨣ��� */
#define KKT_CPI15		0x19		/* ���⭮��� ���� 15 cpi */
#define KKT_ESC			0x1b		/* ��砫� ������� ��� �� */
#define KKT_CPI12		0x1c		/* ���⭮��� ���� 12 cpi */
#define KKT_CPI20		0x1e		/* ���⭮��� ���� 20 cpi */
#define KKT_RST			0x7f		/* ��� �� */

/* ��������⮢� ������� */
#define KKT_FB_BEGIN		0x0c		/* ��砫� �᪠�쭮�� ����� */
#define KKT_FB_END		0x0d		/* ����� �᪠�쭮�� ����� */
#define KKT_TAG_HEADER		0x10		/* ��������� ⥣� */
#define KKT_TAG_VALUE		0x11		/* ���祭�� ⥣� */
#define KKT_STATUS		0x12		/* ����� ����� �� */
/* ���� ������祭��� ���ன�� */
#define KKT_POLL		0x16		/* ��⮬���᪮� ��।������ ���ன�⢠ */
#define KKT_POLL_ENQ		0x7e
#define KKT_POLL_PARAMS		0x7d

/* ��㦥��� ������� */
#define KKT_SRV			0x17		/* �ࢨ᭠� ������� */
/* ����� � ��� */
#define KKT_SRV_FDO_IFACE	0x30		/* ��⠭���� ����䥩� ࠡ��� � ��� */
#define FDO_IFACE_USB		0x30		/* USB (�१ �ନ���) */
#define FDO_IFACE_ETHER		0x31		/* �⥢�� ������ */
#define FDO_IFACE_GPRS		0x32		/* GPRS-����� */
#define FDO_IFACE_OTHER		0x33		/* ����䥩�, ��⠭������� � �� */
#define KKT_SRV_FDO_ADDR	0x31		/* ��⠭���� ip-���� � TCP-���� ��� ࠡ��� � ��� */
#define KKT_SRV_FDO_REQ		0x32		/* ����� ������樨 �� ������ � ��� (USB) */
/* ������樨 �� ������ � ��� (USB) */
#define FDO_REQ_NOP		0x30		/* ������権 ��� */
#define FDO_REQ_OPEN		0x31		/* ��⠭����� ᮥ������� � ��� */
#define FDO_REQ_CLOSE		0x32		/* ࠧ�ࢠ�� ᮥ������� � ��� */
#define FDO_REQ_CONN_ST		0x33		/* ����� ���ﭨ� ᮥ������� � ��� */
#define FDO_REQ_SEND		0x34		/* ��।��� ����� � ��� */
#define FDO_REQ_RECEIVE		0x35		/* ������� ����� �� ��� */
#define FDO_REQ_MESSAGE		0x36		/* ᮮ�饭�� ��� ������ */

#define KKT_SRV_FDO_DATA	0x33		/* �����, �ਭ��� �� ��� (USB) */

/* ��ࠢ����� ��⮪�� ����㠫쭮�� COM-���� */
#define KKT_SRV_FLOW_CTL	0x34		/* ��⠭���� �ࠢ����� ��⮪�� */
#define FCTL_NONE		0x30		/* ��� �ࠢ����� ��⮪�� */
#define FCTL_XON_XOFF		0x31		/* XON/XOFF (�� �����ন������) */
#define FCTL_RTS_CTS		0x32		/* RTS/CTS */
#define FCTL_DTR_DSR		0x33		/* DTR/DSR (�� �����ন������) */

/* ���� ������ ����㠫쭮�� COM-���� */
#define KKT_SRV_RST_TYPE	0x35		/* ��⠭���� ��⮤� ��� ������ */
#define RST_TYPE_NONE		0x30		/* ��� ������ �� �����ন������ */
#define RST_TYPE_BREAK_STATE	0x31		/* ��� ������ �� Break State */
#define RST_TYPE_RTS_CTS	0x32		/* ��� ������ �� RTS/CTS */
#define RST_TYPE_DTR_DSR	0x33		/* ��� ������ �� DTR/DSR */

/* ���� ॠ�쭮�� �६��� (RTC) */
#define KKT_SRV_RTC_GET		0x36		/* ����� �ᮢ ॠ�쭮�� �६��� */
#define KKT_SRV_RTC_SET		0x37		/* ��⠭���� �ᮢ ॠ�쭮�� �६��� */

/* ����� � �� */
#define KKT_SRV_LAST_DOC_INFO	0x38		/* ������� ���ଠ�� � ��᫥���� ��ନ஢����� � ��᫥���� �⯥�⠭��� �᪠�쭮� ���㬥�� */
#define KKT_SRV_PRINT_LAST	0x39		/* ����� ��᫥����� ��ନ஢������, �� ���⯥�⠭���� ���㬥�� */
#define KKT_SRV_BEGIN_DOC	0x3a		/* ����� ���㬥�� */
#define KKT_SRV_SEND_DOC	0x3b		/* ��।�� ������ ���㬥�� */
#define KKT_SRV_END_DOC		0x3c		/* ��ନ஢��� ���㬥�� */
#define KKT_SRV_PRINT_DOC	0x3e		/* ����� ���㬥�� �� ������ */

/* ����ன�� �⥢�� ����䥩ᮢ �� */
#define KKT_SRV_NET_SETTINGS	0x41		/* ����ன�� �� ��� Ethernet */
#define KKT_SRV_GPRS_SETTINGS	0x42		/* ����ன�� GPRS-������ */

/* �મ��� ���� */
#define KKT_SRV_GET_BRIGHTNESS	0x46		/* ��।������ ��ࠬ��஢ �મ�� ���� */
#define KKT_SRV_SET_BRIGHTNESS	0x47		/* ��⠭���� �મ�� ���� */

/* ������� ���� */
#define KKT_WR_BCODE		0x1a		/* ����ᥭ�� ����-���� ��������� ⨯� */
#define KKT_GRID		0x21		/* ����ᥭ�� �⪨ ��� */
#define KKT_TITLE		0x22		/* ��������� ��� */
#define KKT_ROTATE		0x23		/* ������ ⥪�� �� 90 �ࠤ�ᮢ */
#define KKT_ICON		0x24		/* ����� ���⮣ࠬ�� */
#define ICON_DIR_HORIZ		0x7b		/* ��ਧ��⠫쭮� ���ࠢ����� ���� ���⮣ࠬ� (�) */
#define ICON_DIR_VERT		0x65		/* ���⨪��쭮� ���ࠢ����� ���� ���⮣ࠬ� (�) */
#define ICON_ROTATE_0		0x30		/* ����� ���⮣ࠬ�� ��� ������ */
#define ICON_ROTATE_90		0x39		/* ����� ���⮣ࠬ�� � �����⮬ �� 90 �ࠤ�ᮢ ��⨢ �ᮢ�� ��५�� */
#define ICON_IDSET_Z		0x5a		/* �����䨪��� ����� ���⮣ࠬ� (Z) */
#define KKT_PIC			0x25		/* ����� ����ࠦ���� */
#define KKT_CUT_MODE		0x2a		/* ������� ०��� ��१�� ����� */
#define CUT_MODE_FULL		0x30		/* ������ ��१�� */
#define CUT_MODE_PARTIAL	0x31		/* ���筠� ��१�� */
#define KKT_PIC_LOAD		0x48		/* ����㧪� ����ࠦ���� */
#define KKT_PIC_LST		0x49		/* ����祭�� ᯨ᪠ ����㦥���� ����ࠦ���� */
#define KKT_PIC_ERASE		0x4a		/* 㤠����� ����ࠦ���� �� �� */
#define KKT_PIC_FIND		0x4d		/* ���� ����ࠦ���� �� ������ */
#define KKT_POSN		0x5b		/* ������� ����樮��஢���� */
#define KKT_POSN_HPOS_ABS	0x60		/* ��᮫�⭮� ��ਧ��⠫쭮� ����樮��஢���� */
#define KKT_POSN_HPOS_RIGHT	0x61		/* �⭮�⥫쭮� ��ਧ��⠫쭮� ����樮��஢���� ��ࠢ� */
#define KKT_POSN_HPOS_LEFT	0x71		/* �⭮�⥫쭮� ��ਧ��⠫쭮� ����樮��஢���� ����� */
#define KKT_POSN_VPOS_ABS	0x64		/* ��᮫�⭮� ���⨪��쭮� ����樮��஢���� */
#define KKT_POSN_VPOS_FW	0x65		/* �⭮�⥫쭮� ���⨪��쭮� ����樮��஢���� ����� */
#define KKT_POSN_VPOS_BK	0x75		/* �⭮�⥫쭮� ���⨪��쭮� ����樮��஢���� ����� */
#define KKT_LOG			0x6c		/* ��砫� ���� �� */
#define KKT_VF			0x7b		/* ����� ⥪�� ����� */
#define KKT_CHEQUE		0x7e		/* ����� 祪� */
#define KKT_ECHO		0x45		/* �� (��� � ���㬥��樨) */
#define MAX_ECHO_LEN		4000		/* ���ᨬ��쭠� ����� ������ */

/* ������� ��� ࠡ��� � �� */
#define KKT_FS				0x66
#define KKT_FS_STATUS			0x30	/* ����祭�� ����� �� */
#define KKT_FS_NR			0x31	/* ����祭�� ����� �� */
#define KKT_FS_LIFETIME			0x32	/* ����祭�� �ப� ����⢨� �� */
#define KKT_FS_VERSION			0x33	/* ��।������ ���ᨨ �� */
#define KKT_FS_LAST_ERROR		0x35	/* ���ଠ�� �� �訡�� */
#define KKT_FS_SHIFT_PARAMS		0x10	/* ��।������ ��ࠬ��஢ ⥪�饩 ᬥ�� */
#define KKT_FS_TRANSMISSION_STATUS	0x20	/* ����祭�� ����� ���ଠ樮����� ������ */
#define KKT_FS_FIND_DOC			0x40	/* ���� �᪠�쭮�� ���㬥�� �� ������ */
#define KKT_FS_FIND_FDO_ACK		0x41	/* ���� ���⠭樨 ��� �� ������ */
#define KKT_FS_UNCONFIRMED_DOC_CNT	0x42	/* ������� ������⢠ ���㬥�⮢, �� ����� ��� ���⠭樨 ��� */
#define KKT_FS_REGISTRATION_RESULT	0x43	/* ����祭�� �⮣�� �᪠����樨 */
#define KKT_FS_REGISTRATION_RESULT2	0x63	/* ����祭�� �⮣�� �᪠����樨 (��ਠ�� 2) */
#define KKT_FS_REGISTRATION_PARAM	0x44	/* ����祭�� ��ࠬ��� �᪠����樨 */
#define KKT_FS_REGISTRATION_PARAM2	0x64	/* ����祭�� ��ࠬ��� �᪠����樨 (��ਠ�� 2) */
#define KKT_FS_GET_DOC_STLV		0x45	/* ����祭�� ���ଠ樨 �� STLV �᪠�쭮�� ���㬥�� */
#define KKT_FS_READ_DOC_TLV		0x46	/* �⥭�� TLV �᪠�쭮�� ���㬥�� */
#define KKT_FS_READ_REGISTRATION_TLV	0x47	/* �⥭�� TLV ��ࠬ���஢ �᪠����樨 */
#define KKT_FS_LAST_REG_DATA		0x61	/* ����祭�� ��ࠬ��஢ ⥪�饩 ॣ����樨 (������ᨬ� �� ��) */
#define KKT_FS_RESET			0x7f	/* ��� �� */


/* ���������� �� �� (����७��� �������) */
#define KKT_UPDATE		0xa0		/* ���室 � ०�� ���������� */

/* ������ �믮������ ������ */
/* ������� ���� */
#define KKT_STATUS_OK			0x00	/* ��� �訡�� */
#define KKT_STATUS_OK2			0x30	/* ��� �訡�� */
#define KKT_STATUS_PAPER_END		0x41	/* ����� �㬠�� */
#define KKT_STATUS_COVER_OPEN		0x42	/* ���誠 ����� */
#define KKT_STATUS_PAPER_LOCK		0x43	/* �㬠�� �����﫠 �� ��室� */
#define KKT_STATUS_PAPER_WRACK		0x44	/* �㬠�� ���﫠�� */
#define KKT_STATUS_FS_ERR		0x45	/* ���� �����⭠� �訡�� �� */
#define KKT_STATUS_CUT_ERR		0x48	/* �訡�� ��१�� �㬠�� */
#define KKT_STATUS_INIT			0x49	/* �� ��室���� � ���ﭨ� ���樠����樨 */
#define KKT_STATUS_THERMHEAD_ERR	0x4a	/* ��������� �ମ������� */
#define KKT_STATUS_PREV_INCOMPLETE	0x4d	/* �।���� ������� �뫠 �ਭ�� �� ��������� */
#define KKT_STATUS_CRC_ERR		0x4e	/* �।���� ������� �뫠 �ਭ�� � �訡��� ����஫쭮� �㬬� */
#define KKT_STATUS_HW_ERR		0x4f	/* ���� �����⭠� �訡�� �� */
#define KKT_STATUS_NO_FFEED		0x50	/* ��� ������� ��१�� ������ */
#define KKT_STATUS_VPOS_OVER		0x51	/* �ॢ�襭�� ���� ⥪�� �� ���⨪���� ������ */
#define KKT_STATUS_HPOS_OVER		0x52	/* �ॢ�襭�� ���� ⥪�� �� ��ਧ��⠫�� ������ */
#define KKT_STATUS_LOG_ERR		0x53	/* ����襭�� �������� ���ଠ樨 �� ���� �� */
#define KKT_STATUS_GRID_ERROR		0x55	/* ����襭�� ��ࠬ��஢ ����ᥭ�� ����⮢ */
#define KKT_STATUS_BCODE_PARAM		0x70	/* ����襭�� ��ࠬ��஢ ����ᥭ�� ����-���� */
#define KKT_STATUS_NO_ICON		0x71	/* ���⮣ࠬ�� �� ������� */
#define KKT_STATUS_GRID_WIDTH		0x72	/* �ਭ� �⪨ ����� �ਭ� ������ � ��⠭����� */
#define KKT_STATUS_GRID_HEIGHT		0x73	/* ���� �⪨ ����� ����� ������ � ��⠭����� */
#define KKT_STATUS_GRID_NM_FMT		0x74	/* ���ࠢ���� �ଠ� ����� �⪨ */
#define KKT_STATUS_GRID_NM_LEN		0x75	/* ����� ����� �⪨ ����� �����⨬�� */
#define KKT_STATUS_GRID_NR		0x76	/* ������ �ଠ� ����� �⪨ */
#define KKT_STATUS_INVALID_ARG		0x77	/* ������ ��ࠬ��� */

/* ������� ���� ���ன�⢠ */
#define POLL_STATUS_OK			KKT_STATUS_OK
#define POLL_STATUS_PARAM_ERR		0x80	/* �訡��� ��ࠬ���� */

/* ������� ��⠭���� ����䥩� ࠡ��� � ��� */
#define FDO_IFACE_STATUS_OK		KKT_STATUS_OK	/* ����䥩� ��⠭����� */
#define FDO_IFACE_STATUS_NO		0x41	/* � �� ��� ⠪��� ����䥩� */

/* ������� ��⠭���� ip-���� � TCP-���� ��� ࠡ��� � ��� */
#define FDO_ADDR_STATUS_OK		KKT_STATUS_OK	/* ���� ��� ��⠭����� */
#define FDO_ADDR_STATUS_BAD_IP		0x41	/* �������⨬� ip */
#define FDO_ADDR_STATUS_BAD_PORT	0x42	/* �������⨬� ����� ���� */

/* ������� ����樨 � ��� */
#define FDO_OPEN_ERROR			0x00	/* �訡�� ᮥ������� � ��� */
#define FDO_OPEN_CONNECTION_STARTED	0x01	/* ᮥ������� � ��� ���� */
#define FDO_OPEN_BAD_ADDRESS		0x02	/* ������ �ଠ� ���� ��� */
#define FDO_OPEN_ALREADY_CONNECTED	0x03	/* ᮥ������� � ��� ��⠭������ ࠭�� */
#define FDO_OPEN_CONNECTED		0x04	/* ᮥ������� � ��� ��⠭������ */
#define FDO_CLOSE_ERROR			0x00	/* �訡�� ������� ᮥ������� � ��� */
#define FDO_CLOSE_STARTED		0x01	/* ��砫��� �����⨥ ᮥ������� � ��� */
#define FDO_CLOSE_NOT_CONNECTED		0x02	/* ��� ᮥ������� � ��� */
#define FDO_CLOSE_COMPLETE		0x03	/* ᮥ������� � ��� ������ */
#define FDO_STATUS_NOT_CONNECTED	0x00	/* ��� ᮥ������� � ��� */
#define FDO_STATUS_CONNECTED		0x01	/* ���� ᮥ������� � ��� */
#define FDO_SEND_ERROR			0x00	/* �訡�� ��।�� ������ � ��� */
#define FDO_SEND_OK			0x01	/* ����� ��।��� � ��� */
#define FDO_SEND_DATA_ERROR		0x02	/* �訡�� ����祭�� ������ ��� ��� �� �� */
#define FDO_SEND_NOT_CONNECTED		0x03	/* ��� ᮥ������� � ��� */
#define FDO_RCV_NO_DATA			0x00	/* ��� ������ �� ��� */
#define FDO_RCV_OK			0x01	/* ����� �� ��� ��।��� � �� */
#define FDO_RCV_NOT_CONNECTED		0x02	/* ��� ᮥ������� � ��� */
#define FDO_MSG_SAVE_ERROR		0x00	/* �� 㤠���� ��࠭��� ᮮ�饭�� ��� ������ */
#define FDO_MSG_OK			0x01	/* ᮮ�饭�� ��� ������ �ਭ�� */
#define FDO_MSG_RCV_ERROR		0x02	/* �訡�� ��� ᮮ�饭�� ��� ������ */

/* ������� ����祭�� ������, �ਭ���� �� ��� (USB) */
#define FDO_DATA_STATUS_OK		KKT_STATUS_OK2	/* ����� �ᯥ譮 �ਭ��� */
#define FDO_DATA_STATUS_FAULT		0x31	/* ����� �� 㤠���� �ਭ��� */

/* ������� ��⠭���� �ࠢ����� ��⮪�� */
#define FLOW_CTL_STATUS_OK		KKT_STATUS_OK2	/* �ࠢ����� ��⮪�� ��⠭������ */
#define FLOW_CTL_STATUS_FAULT		0x31	/* 㪠����� ⨯ �ࠢ����� ��⮪�� ��⠭����� �� 㤠���� */
#define FLOW_CTL_STATUS_UNSUPPORTED	0x32	/* 㪠����� ⨯ �ࠢ����� ��⮪�� �� �����ন������ */

/* ������� ��⠭���� ��⮤� ��� ������ */
#define RST_TYPE_STATUS_OK		KKT_STATUS_OK2	/* ⨯ ��� ������ ��⠭����� */
#define RST_TYPE_STATUS_FAULT		0x31	/* ⨯ ��� ������ ��⠭����� �� 㤠���� */
#define RST_TYPE_STATUS_UNSUPPORTED	0x32	/* ⨯ ��� ������ �� �����ন������ */

/* ������� ����� �ᮢ ॠ�쭮�� �६��� */
#define RTC_GET_STATUS_OK		KKT_STATUS_OK2	/* ����� ����祭� */
#define RTC_GET_STATUS_FAULT		0x31	/* �訡�� ����祭�� ���ଠ樨 �� RTC */

/* ������� ��⠭���� �ᮢ ॠ�쭮�� �६��� */
#define RTC_SET_STATUS_OK		KKT_STATUS_OK2	/* RTC ��⠭������ */
#define RTC_SET_STATUS_FMT		0x31	/* �訡�� �ଠ� ������ */
#define RTC_SET_STATUS_FAULT		0x32	/* �訡�� ��⠭���� RTC */

/* ������� ����祭�� ���ଠ樨 � ��᫥���� ��ନ஢����� � ��᫥���� �⯥�⠭��� �᪠�쭮� ���㬥�� */
#define LAST_DOC_INFO_STATUS_OK		KKT_STATUS_OK2	/* ���ଠ�� ����祭� */
#define LAST_DOC_INFO_STATUS_FAULT	0x31	/* ���ଠ�� �� ����祭� */

/* ������� ���� ��᫥����� ��ନ஢������, �� ���⯥�⠭���� �᪠�쭮�� ���㬥�� */
#define PRINT_LAST_STATUS_OK		KKT_STATUS_OK2	/* ���㬥�� �⯥�⠭ */
#define PRINT_LAST_STATUS_FORM_UNKNOWN	0x31	/* ������ ��� ��� �᪠�쭮�� ���㬥�� */
#define PRINT_LAST_STATUS_FORM_MISMATCH	0x32	/* ��ᮢ������� ���� ��� �᪠�쭮�� ���㬥�� */
#define PRINT_LAST_STATUS_TEMPLATE_FMT	0x33	/* �訡�� �������� 蠡���� */
#define PRINT_LAST_STATUS_OVERFLOW	0x34	/* �������筮 ����� ��� ��� 蠡���� */
#define PRINT_LAST_STATUS_RCV_FAIL	0x35	/* �訡�� ��� 蠡���� */
#define PRINT_LAST_STATUS_PRN_FAIL	0x36	/* �訡�� ���� */
#define PRINT_LAST_STATUS_NO_PRN_FLAG	0x37	/* 䫠� ���� ���㬥�� �� ��⠭����� */

/* ������� ��⠭���� �⥢�� ����஥� Ethernet */
#define NET_SETTINGS_STATUS_OK		KKT_STATUS_OK2	/* ����ன�� �ᯥ譮 ��⠭������ */
#define NET_SETTINGS_STATUS_BAD_IP	0x31	/* ������ �ଠ� ip-���� �� */
#define NET_SETTINGS_STATUS_BAD_NETMASK	0x32	/* ������ �ଠ� ��᪨ ����� */
#define NET_SETTINGS_STATUS_BAD_GW	0x33	/* ������ �ଠ� ip-���� � */

/* ������� ��⠭���� ����஥� GPRS */
#define GPRS_CFG_STATUS_OK		KKT_STATUS_OK2	/* ����ன�� GPRS �ᯥ譮 ��⠭������ */
#define GPRS_CFG_STATUS_APN_LEN		0x31	/* ����� APN ����� �����⨬�� */
#define GPRS_CFG_STATUS_APN_FMT		0x32	/* �������⨬� ᨬ���� � APN */
#define GPRS_CFG_STATUS_USER_LEN	0x33	/* ����� ����� ���짮��⥫� ����� �����⨬�� */
#define GPRS_CFG_STATUS_USER_FMT	0x34	/* �������⨬� ᨬ���� � ����� ���짮��⥫� */
#define GPRS_CFG_STATUS_PASSWORD_LEN	0x35	/* ����� ��஫� ����� �����⨬�� */
#define GPRS_CFG_STATUS_PASSWORD_FMT	0x36	/* �������⨬� ᨬ���� � ��஫� */

/* ������� �믮������ ������� �� */
#define ECHO_STATUS_OK			KKT_STATUS_OK2	/* ��� �訡�� */
#define ECHO_STATUS_OVERFLOW		0x31	/* ����� �� ��������� � ���� */

/* �������⥫�� ������ (�� �ᯮ������� � ��) */
#define KKT_STATUS_INTERNAL_BASE	0x80	/* ��砫� �������⥫��� ����ᮢ */
#define KKT_STATUS_UPDT_NOT_READY	0x90	/* �� ��⮢ � ���������� */
#define KKT_STATUS_UPDT_ERASE_FAULT	0x91	/* �訡�� ��࠭�� ������ FLASH */
#define KKT_STATUS_UPDT_READ_ERR	0x92	/* �訡�� �⥭�� ������ �� FLASH */
#define KKT_STATUS_UPDT_WRITE_ERR	0x93	/* �訡�� ����� ������ �� FLASH */
#define KKT_STATUS_UPDT_CHECK_FAULT	0x94	/* ��ᮢ������� ������ �� �஢�થ */
#define KKT_STATUS_UPDT_RESP_FMT	0x95	/* ������ �ଠ� �⢥� ����஫��� */
#define KKT_STATUS_UPDT_FC_FMT		0x96	/* ������ �ଠ� �᪠�쭮�� �� */
#define KKT_STATUS_WR_OVERFLOW		0xf0	/* ��९������� ���� ��।�� */
#define KKT_STATUS_OP_TIMEOUT		0xf1	/* ⠩���� ����樨 */
#define KKT_STATUS_COM_ERROR		0xf2	/* �訡�� COM-���� */
#define KKT_STATUS_RESP_FMT_ERROR	0xf3	/* ������ �ଠ� �⢥� */
#define KKT_STATUS_MUTEX_ERROR		0xf4	/* �訡�� ����祭�� ���쥪� */
#define KKT_STATUS_OVERFLOW		0xf5	/* ��९������� ���� */

/* �������� � ���� ᥪ ����権 � ��� */
extern uint32_t kkt_base_timeout;
#define KKT_BASE_TIMEOUT		10
#define KKT_DEF_TIMEOUT			10 * kkt_base_timeout	// 100
#define KKT_STATUS_TIMEOUT		kkt_base_timeout	// 10
#define KKT_FDO_IFACE_TIMEOUT		10 * kkt_base_timeout	// 100
#define KKT_FDO_ADDR_TIMEOUT		10 *kkt_base_timeout	// 100
#define KKT_FDO_DATA_TIMEOUT		30 * kkt_base_timeout	// 300
#define KKT_FR_STATUS_TIMEOUT		kkt_base_timeout	// 10
#define KKT_FR_PRINT_TIMEOUT		50 * kkt_base_timeout	// 500
#define KKT_FR_RESET_TIMEOUT		200 * kkt_base_timeout	// 2000

#endif		/* KKT_CMD_H */
