/* Работа с дополнительным принтером. (c) gsr 2004-2005 */

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "prn/aux.h"
#include "serial.h"
#include "sterm.h"
#include "gui/scr.h"
#include "gui/status.h"

static int aprn_dev = -1;		/* устройство дополнительного принтера */

/* Вызывается из init_term */
void aprn_init(void)
{
	static char aprn_dev_name[] = "/dev/ttyS0";
	static int l = -1;
	struct serial_settings ss = {
		.csize		= CS8,
		.parity		= SERIAL_PARITY_NONE,
		.stop_bits	= SERIAL_STOPB_1,
		.control	= SERIAL_FLOW_RTSCTS,
		.baud		= B19200
	};
	if (l == -1)
		l = strlen(aprn_dev_name);
	aprn_release();
	if (cfg.has_aprn){
		aprn_dev_name[l - 1] = cfg.aprn_tty + 0x30;
		aprn_dev = serial_open(aprn_dev_name, &ss, O_WRONLY);
		if (aprn_dev == -1)
			cfg.has_aprn = false;
	}
}

void aprn_release(void)
{
	if (aprn_dev != -1){
		tcflush(aprn_dev, TCIOFLUSH);
		close(aprn_dev);
		aprn_dev = -1;
	}
}

void aprn_flush(void)
{
	prn_buf_len = 0;
}

static int aprn_write_text(const char *txt, int len)
{
	if ((txt == NULL) || (len <= 0) || (len > PRN_BUF_LEN))
		return 0;
	memcpy(prn_buf, txt, len);
	return prn_buf_len = len;
}

static int aprn_out_char(uint8_t ch)
{
#if defined __DEBUG_PRINT__
	putchar(ch & 0x7f);
	return prn_ready;
#else
/* Попытка передачи символа в принтер */
	ch &= 0x7f;
	switch (write(aprn_dev, &ch, 1)){
		case 1:
			return prn_ready;
		case 0:
			return prn_dead;
		default:
			if (errno != -EAGAIN)
				return prn_dead;
			else
				return prn_pout;
	}
#endif
}

bool aprn_print(const char *txt, int l)
{
	uint8_t *ptr = prn_buf;
	int i = 0, lsr;
	bool lflag = true,
#if defined __DEBUG_PRINT__
	cts = true;
#else
	said = false, cts;
#endif
	if (aprn_write_text(txt, l) == 0)
		return true;
	set_term_astate(ast_none);
	while (lflag){
		if (prn_buf_len == 0)
			return false;			/* имел место сброс терминала */
#if !defined __DEBUG_PRINT__
		cts = (serial_get_lines(aprn_dev) & TIOCM_CTS) != 0;
		if (cts){
/*			tcflow(aprn_dev, TCOON);*/	/* возобновить передачу по COM-порту */
			if (said){
				set_term_astate(ast_none);
				said = false;
			}
		}else if (i < prn_buf_len){
/*			tcflow(aprn_dev, TCOOFF);*/	/* приостановить передачу по COM-порту */
			if (!said){
				set_term_astate(ast_noaprn);
				said = true;
			}
		}
#endif
		if (cts && (i < prn_buf_len) && (kt != key_none)){
			if (aprn_out_char(ptr[i] & 0x7f) == prn_ready)
				i++;
		}else if (i >= prn_buf_len){
			if (ioctl(aprn_dev, TIOCSERGETLSR, &lsr) == 0)
				lflag = !(lsr & TIOCSER_TEMT);
		}
		if ((get_cmd(false, true) == cmd_reset) && reset_term(false)){
			aprn_flush();
			return false;
		}
	}
	prn_buf_len = 0;
	return true;
}
