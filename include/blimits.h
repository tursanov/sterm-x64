/* Размеры основных буферов терминала. (c) gsr 2004, 2018 */

#if !defined BLIMITS_H
#define BLIMITS_H

#define OUT_BUF_LEN		2048	/* ОЗУ заказа */
#define REQ_BUF_LEN		4096	/* буфер для передачи заказа */
#define RESP_BUF_LEN		65536	/* ОЗУ ответа */
#define SCR_RESP_BUF_LEN	4096	/* размер в словах экранного буфера ответа */
#define TEXT_BUF_LEN		16384	/* буфер абзаца ответа */
#define KEY_BUF_LEN		16384	/* ОЗУ ключей */
#define PRN_BUF_LEN		8192	/* ОЗУ печати */
#define PROM_BUF_LEN		3200	/* ДЗУ */
#define ROM_BUF_LEN		16384	/* ОЗУ констант */
#define STUFF_BUF_LEN   	20480	/* буфер просмотра обмена */
#define MAX_PARAS		1000	/* максимальное число абзацев в принятом ответе */
#define XLOG_MAX_SIZE		2097152	/* максимальный размер файла ЦКЛ без учёта заголовка */
#define PLOG_MAX_SIZE		2097152	/* максимальный размер файла БКЛ без учёта заголовка */
#define LLOG_MAX_SIZE		2097152	/* максимальный размер файла ПКЛ без учёта заголовка */
#define KLOG_MAX_SIZE		2097152	/* максимальный размер файла ККЛ без учёта заголовка */
#define LOG_BUF_LEN		8192	/* размер буфера данных КЛ */
#define GUI_KLOG_BUF_LEN	32768	/* размер экранного буфера записи ККЛ */

#define MAX_WINDOWS		10	/* максимальное число окон ОЗУ заказа */

/* Строка статуса */
#define MAX_TERM_STATE_LEN	20
#define MAX_TERM_ASTATE_LEN	25
#define TERM_STATUS_LEN		81

#endif		/* BLIMITS_H */
