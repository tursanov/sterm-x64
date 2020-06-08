/*
 * Автоматическое определение типа адаптера BSC.
 * В модуле chipset находятся все функции, специфичные
 * для конкретного адаптера.
 * (c) gsr & В.Г.Афанасьев 2003
 */

#if !defined SYS_CHIPSET_H
#define SYS_CHIPSET_H

#include "sysdefs.h"

/* Тип используемого адаптера BSC */
enum {
	chipsetUNKNOWN = 0,	/* неизвестный */
	chipsetCAVE,		/* на основе КР580ВВ55А (старый вариант) */
	chipsetPPIO,		/* на основе КР580ВВ55А */
	chipsetXILINX,		/* на основе XILINX */
	chipsetPCI,		/* адаптер PCI */
	chipsetCARDLESS,	/* без адаптера BSC */
	chipsetSTRIDER,		/* Win32 */
};

/*
 * Используемый адаптер описывается специальной структурой,
 * в которой содержатся все данные, необходимые для работы с ним.
 * В модуле ядра соответствующие поля структуры используются напрямую,
 * а в пользовательских программах -- через IOCTL.
 */

/* Прототипы функций для работы с адаптером */
/* BSC */
/* Чтение байта данных */
typedef uint8_t (*bsc_read_dta_t)(void);
/* Запись байта данных */
typedef void (*bsc_write_dta_t)(uint8_t);
/* Чтение регистра состояния */
typedef uint8_t (*bsc_read_ctl_t)(void);
/* Запись регистра состояния */
typedef void (*bsc_write_ctl_t)(uint8_t);
/* Чтение состояния линии ГОТОВНОСТЬ коммутатора */
typedef bool (*bsc_GOT_t)(void);
/* Установка линии ЗАКАЗ коммутатора */
typedef void (*bsc_ZAK_t)(bool);
/* Принтер */
/* Запись байта данных */
typedef void (*prn_write_data_t)(uint8_t);
/* Чтение шины BUSY */
typedef bool (*prn_BUSY_t)(void);
/* Чтение шины DRQ (ACK) */
typedef bool (*prn_DRQ_t)(void);
/* Установка шины RPR */
typedef void (*prn_RPR_t)(bool);
/* Установка шины SPR */
typedef void (*prn_SPR_t)(bool);
/* Ключи DS1990A */
/* Установка линии DLSO */
typedef void (*dallas_DLSO_t)(bool);
/* Чтение линии DLSK */
typedef bool (*dallas_DLSK_t)(void);

/* Описание адаптера */
struct chipset_metrics {
/* Общие характеристики */
	char *name;		/* название адаптера */
	uint32_t io_base;	/* начало диапазона ввода/вывода */
	uint32_t io_extent;	/* размер дипазона ввода/вывода */
/* BSC */
	int bsc_irq;
	bsc_read_dta_t bsc_read_dta;
	bsc_write_dta_t bsc_write_dta;
	bsc_read_ctl_t bsc_read_ctl;
	bsc_write_ctl_t bsc_write_ctl;
	bsc_GOT_t bsc_GOT;
	bsc_ZAK_t bsc_ZAK;
/* Принтер */
	prn_write_data_t prn_write_data;
	prn_BUSY_t prn_BUSY;
	prn_DRQ_t prn_DRQ;
	prn_RPR_t prn_RPR;
	prn_SPR_t prn_SPR;
/* Ключи DS1990A */
	dallas_DLSO_t dallas_DLSO;
	dallas_DLSK_t dallas_DLSK;
};

extern int chipset_type;
extern struct chipset_metrics cm;

/* Функция проверки соответствующего адаптера */
typedef bool (*chipset_probe_t)(void);
/* Функция установки параметров соответствующего адаптера */
typedef void (*chipset_set_metrics_t)(struct chipset_metrics *);

#endif		/* SYS_CHIPSET_H */
