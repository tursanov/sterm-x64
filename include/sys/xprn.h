#if !defined SYS_XPRN_H
#define SYS_XPRN_H

/* Базовый адрес ввода-вывода для LPT TL16C552 */
#define PXA_LPT_MMIO_PHYS	PXA_CS4_PHYS + 0x200

/* Смещения регистров LPT TL16C552 */
#define PXA_LPT_DR		0	/* регистр данных (чтение/запись) */
#define PXA_LPT_SR		4	/* регистр статуса (чтение) */
#define PXA_LPT_CR		8	/* регистр управления (чтение/запись) */

/* Биты регистра статуса */
#define PXA_LPT_SR_BUSY		0x80	/* Busy# */
#define PXA_LPT_SR_ACK		0x40	/* Ack# */
#define PXA_LPT_SR_PE		0x20	/* Paper End */
#define PXA_LPT_SR_SLCT		0x10	/* Select */
#define PXA_LPT_SR_ERR		0x08	/* Error# */
#define PXA_LPT_SR_PIRQ		0x04	/* Print Interrupt */

/* Биты регистра управления */
#define PXA_LPT_CR_DIR		0x20	/* 1 -- режим ввода, иначе вывод */
#define PXA_LPT_CR_IEN		0x10	/* разрешение прерывания */
#define PXA_LPT_CR_SLCT_IN	0x08	/* SelectIn */
#define PXA_LPT_CR_INIT		0x04	/* Init# */
#define PXA_LPT_CR_ALF		0x02	/* Auto LF */
#define PXA_LPT_CR_STB		0x01	/* Strobe */

enum {
	prn_ready,
	prn_pout,
	prn_dead
};

#endif		/* SYS_XPRN_H */
