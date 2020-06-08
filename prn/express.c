/* Работа с принтером "Экспресс". (c) gsr 2000, 2004 */

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include <unistd.h>
#include "gui/scr.h"
#include "gui/status.h"
#include "prn/express.h"
#include "sterm.h"

static int xprn = -1;
static bool need_persist = false;
static uint8_t pcmd[32];
static int pcmd_len = 0;

void xprn_init(void)
{
	xprn = open(XPRN_DEV_NAME, O_RDONLY);
	if (xprn != -1)
		xprn_flush();
}

void xprn_release(void)
{
	if (xprn != -1){
		close(xprn);
		xprn = -1;
	}
}

void xprn_flush(void)
{
	if (xprn != -1)
		ioctl(xprn, XPRN_IO_RESET);
	prn_buf_len = 0;
	need_persist = false;
}

/* Подготовка текста к печати */
static int xprn_write_text(const char *p, int l)
{
	enum {
		st_start,
		st_normal,
		st_dle,
		st_fnt,
		st_vpos,
		st_bcode1,
		st_bcode2,
		st_bcode3,
		st_err,
	};
	static struct {
		uint8_t n;
		uint8_t cmd[2];
	} fc[] = {
		{0x30, {XPRN_CPI10, XPRN_NORM_FNT}},
		{0x31, {XPRN_CPI10, XPRN_WIDE_FNT}},
		{0x32, {XPRN_CPI10, XPRN_ITALIC}},
		{0x33, {XPRN_CPI12, XPRN_NORM_FNT}},
		{0x34, {XPRN_CPI12, XPRN_WIDE_FNT}},
		{0x35, {XPRN_CPI12, XPRN_ITALIC}},
		{0x36, {XPRN_CPI16, XPRN_NORM_FNT}},
		{0x37, {XPRN_CPI16, XPRN_WIDE_FNT}},
		{0x38, {XPRN_CPI16, XPRN_ITALIC}},
	};
	static struct {
		uint8_t n;
		uint8_t cmd[5];
	} vc[] = {
		{0x30, {XPRN_DLE1, XPRN_PRNOP, 0x31, 0x34, XPRN_PRNOP_VPOS_BK}},
		{0x31, {XPRN_DLE1, XPRN_PRNOP, 0x31, 0x32, XPRN_PRNOP_VPOS_BK}},
		{0x32, {XPRN_DLE1, XPRN_PRNOP, 0x31, 0x30, XPRN_PRNOP_VPOS_BK}},
		{0x33, {XPRN_DLE1, XPRN_PRNOP, 0x30, 0x38, XPRN_PRNOP_VPOS_BK}},
		{0x34, {XPRN_DLE1, XPRN_PRNOP, 0x30, 0x36, XPRN_PRNOP_VPOS_BK}},
		{0x35, {XPRN_DLE1, XPRN_PRNOP, 0x30, 0x34, XPRN_PRNOP_VPOS_BK}},
		{0x36, {XPRN_DLE1, XPRN_PRNOP, 0x30, 0x32, XPRN_PRNOP_VPOS_BK}},
		{0x37, {XPRN_DLE1, XPRN_PRNOP, 0x30, 0x30, XPRN_PRNOP_VPOS_BK}},
		{0x38, {XPRN_DLE1, XPRN_PRNOP, 0x30, 0x32, XPRN_PRNOP_VPOS_FW}},
		{0x39, {XPRN_DLE1, XPRN_PRNOP, 0x30, 0x34, XPRN_PRNOP_VPOS_FW}},
		{0x41, {XPRN_DLE1, XPRN_PRNOP, 0x30, 0x36, XPRN_PRNOP_VPOS_FW}},
		{0x42, {XPRN_DLE1, XPRN_PRNOP, 0x30, 0x38, XPRN_PRNOP_VPOS_FW}},
		{0x43, {XPRN_DLE1, XPRN_PRNOP, 0x31, 0x30, XPRN_PRNOP_VPOS_FW}},
		{0x44, {XPRN_DLE1, XPRN_PRNOP, 0x31, 0x32, XPRN_PRNOP_VPOS_FW}},
		{0x45, {XPRN_DLE1, XPRN_PRNOP, 0x31, 0x34, XPRN_PRNOP_VPOS_FW}},
		{0x46, {XPRN_DLE1, XPRN_PRNOP, 0x31, 0x36, XPRN_PRNOP_VPOS_FW}},
	};
	int i, n = 0, st = st_start;
	uint8_t b;
	if ((p == NULL) || (l > sizeof(prn_buf)))
		return 0;
	prn_buf_len = 0;
	memset(pcmd, 0, sizeof(pcmd));
	pcmd[0] = XPRN_NUL;
	pcmd_len = 0;
	need_persist = false;
	for (i = -1; (st != st_err) && (i < l) && (prn_buf_len < sizeof(prn_buf)); i++){
		b = p[i];
		switch (st){
			case st_start:
				prn_buf[prn_buf_len++] = 0;
				st = st_normal;
				break;
			case st_normal:
				if (is_escape(b))
					st = st_dle;
				else
					prn_buf[prn_buf_len++] = b;
				break;
			case st_dle:
				switch (b){
					case XPRN_FONT:
						st = st_fnt;
						break;
					case XPRN_VPOS:
						st = st_vpos;
						break;
					case XPRN_RD_BCODE:
						if (need_persist){
							if ((prn_buf_len + 2) <= sizeof(prn_buf)){
								memcpy(prn_buf + prn_buf_len, p + i - 1, 2);
								prn_buf_len += 2;
								st = st_normal;
							}else
								st = st_err;
						}else{
							memcpy(pcmd + 1, p + i - 1, 2);
							pcmd_len = 3;
							st = st_bcode1;
						}
						break;
					case XPRN_NO_BCODE:
						if (need_persist){
							if ((prn_buf_len + 2) <= sizeof(prn_buf)){
								memcpy(prn_buf + prn_buf_len, p + i - 1, 2);
								prn_buf_len += 2;
								st = st_normal;
							}else
								st = st_err;
						}else{
							memcpy(pcmd + 1, p + i - 1, 2);
							pcmd_len = 3;
							need_persist = true;
						}
						st = st_normal;
						break;
					default:
						if ((prn_buf_len + 2) <= sizeof(prn_buf)){
							memcpy(prn_buf + prn_buf_len, p + i - 1, 2);
							prn_buf_len += 2;
							st = st_normal;
						}else
							st = st_err;
						break;
				}
				break;
			case st_fnt:
				for (n = 0; n < ASIZE(fc); n++){
					if (fc[n].n == b){
						if ((prn_buf_len + sizeof(fc[n].cmd)) <= sizeof(prn_buf)){
							memcpy(prn_buf + prn_buf_len,
								fc[n].cmd, sizeof(fc[n].cmd));
							prn_buf_len += sizeof(fc[n].cmd);
						}else
							st = st_err;
						break;
					}
				}
				if (st != st_err)
					st = st_normal;
				break;
			case st_vpos:
				for (n = 0; n < ASIZE(vc); n++){
					if (vc[n].n == b){
						if ((prn_buf_len + sizeof(vc[n].cmd)) <= sizeof(prn_buf)){
							memcpy(prn_buf + prn_buf_len,
								vc[n].cmd, sizeof(vc[n].cmd));
							prn_buf_len += sizeof(vc[n].cmd);
						}else
							st = st_err;
						break;
					}
				}
				if (st != st_err)
					st = st_normal;
				break;
			case st_bcode1:
				pcmd[pcmd_len++] = b;
				if (b == ';')
					n = 0;
				else
					n = 1;
				st = st_bcode2;
				break;
			case st_bcode2:
				pcmd[pcmd_len++] = b;
				if (n > 0)
					n--;
				else{
					st = st_bcode3;
					if (b == ';')
						n = 13;
					else
						n = 14;
				}
				break;
			case st_bcode3:
				pcmd[pcmd_len++] = b;
				if (n > 0)
					n--;
				if (n == 0){
					need_persist = true;
					st = st_normal;
				}
				break;
		}
	}
	if ((st == st_normal) && (i == l))
		return prn_buf_len;
	else
		return -1;
}

bool xprn_printing = false;

/* Печать текста на ОПУ */
bool xprn_print(const char *txt, int l)
{
	bool said = false;
	uint8_t *ptr = prn_buf;
	int i = 0, ii = 0, rc = prn_ready;
	bool ch_buf = false;
	bool lflag = true;
	if (xprn_write_text(txt,l) <= 0)
		return true;
	ch_buf = need_persist;
	set_term_astate(ast_none);
	xprn_printing = true;
	bool ret = true;
	while (lflag){
		if (prn_buf_len == 0){		/* имел место сброс терминала */
			ret = false;
			break;
		}
		if (ch_buf){			/* смена текущего буфера */
			if (ptr != pcmd){
				ptr = pcmd;
				ii = i;
			}
			i = 0;
			ch_buf = false;
		}
		if ((kt != key_none)){
#if defined __DEBUG_PRINT__
			putchar(ptr[i] & 0x7f);
#else
			rc = ioctl(xprn, XPRN_IO_OUTCHAR, ptr[i] & 0x7f);
#endif
			switch (rc){
				case prn_ready:
					i++;
					if (said){
						set_term_astate(ast_none);
						said = false;
					}
					if (ptr == prn_buf){
						if (i == prn_buf_len)
							lflag = false;	/* конец буфера печати */
					}else if (i == pcmd_len){
						ptr = prn_buf;		/* конец "постоянной" команды */
						i = ii;
					}
					break;
				case prn_dead:
					ch_buf = need_persist;		/* вновь вывести "постоянную" команду */
					if (!said){
						set_term_astate(ast_noxprn);
						said = true;
					}
					break;
			}
		}
		if ((get_cmd(false, true) == cmd_reset) && reset_term(false)){
			xprn_flush();
			ret = false;
			break;
		}
	}
	prn_buf_len = 0;
	if (ret)
		usleep(500000);
	xprn_printing = false;
	return ret;
}
