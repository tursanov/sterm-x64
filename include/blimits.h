/* ������� �᭮���� ���஢ �ନ����. (c) gsr 2004, 2018 */

#if !defined BLIMITS_H
#define BLIMITS_H

#if defined __cplusplus
extern "C" {
#endif

#define OUT_BUF_LEN		2048	/* ��� ������ */
#define REQ_BUF_LEN		4096	/* ���� ��� ��।�� ������ */
#define RESP_BUF_LEN		65536	/* ��� �⢥� */
#define SCR_RESP_BUF_LEN	4096	/* ࠧ��� � ᫮��� �࠭���� ���� �⢥� */
#define TEXT_BUF_LEN		16384	/* ���� ����� �⢥� */
#define KEY_BUF_LEN		16384	/* ��� ���祩 */
#define PRN_BUF_LEN		8192	/* ��� ���� */
#define PROM_BUF_LEN		3200	/* ��� */
#define ROM_BUF_LEN		16384	/* ��� ����⠭� */
#define STUFF_BUF_LEN   	20480	/* ���� ��ᬮ�� ������ */
#define MAX_PARAS		1000	/* ���ᨬ��쭮� �᫮ ����楢 � �ਭ�⮬ �⢥� */
#define XLOG_MAX_SIZE		2097152	/* ���ᨬ���� ࠧ��� 䠩�� ��� ��� ���� ��������� */
#define PLOG_MAX_SIZE		2097152	/* ���ᨬ���� ࠧ��� 䠩�� ��� ��� ���� ��������� */
#define LLOG_MAX_SIZE		2097152	/* ���ᨬ���� ࠧ��� 䠩�� ��� ��� ���� ��������� */
#define KLOG_MAX_SIZE		2097152	/* ���ᨬ���� ࠧ��� 䠩�� ��� ��� ���� ��������� */
#define LOG_BUF_LEN		8192	/* ࠧ��� ���� ������ �� */
#define GUI_KLOG_BUF_LEN	32768	/* ࠧ��� �࠭���� ���� ����� ��� */

#define MAX_WINDOWS		10	/* ���ᨬ��쭮� �᫮ ���� ��� ������ */

/* ��ப� ����� */
#define MAX_TERM_STATE_LEN	20
#define MAX_TERM_ASTATE_LEN	25
#define TERM_STATUS_LEN		81

#if defined __cplusplus
}
#endif

#endif		/* BLIMITS_H */
