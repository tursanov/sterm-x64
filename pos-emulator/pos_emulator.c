/*
 * pos_emulator.c ? эмулятор ИПТ для ПАК РМК АСУ ?Экспресс-3?.
 *
 * Создаёт PTY, симлинк в /dev/, читает команды от терминала
 * и отвечает согласно протоколу (см. ipt-2026-*.txt).
 *
 * Для MTYPE 0xA3/0xA4/0xA5 эмулятор отправляет экранную форму
 * с EDIT-полями RESULT_CODE / RESPONSE_CODE / INVOICE и кнопками
 * ОТПРАВИТЬ / ОТМЕНА. Значения, введённые оператором в терминале,
 * попадают в RESPONSE PARAMETERS сценария FINISH MENU.
 *
 * Сборка:  gcc -O2 -Wall -Wextra -std=gnu11 -o pos_emulator pos_emulator.c -lutil
 * Запуск:  sudo ./pos_emulator /dev/ttyS3
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pty.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <termios.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>

/* ============================ Константы ============================ */

#define DEV_LINK_DEFAULT "/dev/ttyIPT"
#define RX_CAP           262144
#define MAX_BLOCKS       16
#define BLOCK_CAP        8192

/* Потоки (по реальным логам ipt-2026-*.txt) */
#define STREAM_KBD       0x0000
#define STREAM_SCREEN    0x0001
#define STREAM_PRINTER   0x0002
#define STREAM_TCP       0x0003
#define STREAM_CMD       0x0099
#define STREAM_ERR       0x0005

/* Команды ?Потока команд? */
#define CMD_INIT             0x01
#define CMD_FINISH           0x02
#define CMD_REQUEST_PARAMS   0x03
#define CMD_RESPONSE_PARAMS  0x04
#define CMD_INIT_REQUIRED    0x05
#define CMD_INIT_CHECK       0x06

/* Экранные команды */
#define SCR_CUR   0x01
#define SCR_CLS   0x02
#define SCR_EDIT  0x03
#define SCR_MENU  0x04
#define SCR_PRINT 0x05
#define SCR_COLOR 0x08

/* MTYPE */
#define MT_UNFINISHED  0xA0
#define MT_OPEN_DAY    0xA1
#define MT_CLOSE_DAY   0xA2
#define MT_PAY         0xA3
#define MT_RETURN      0xA4
#define MT_CANCEL      0xA5
#define MT_SERVICE     0xA6

/* RESULT_CODE */
#define RC_OK              0x00
#define RC_UNFINISHED      0x01
#define RC_ABORTED         0x02
#define RC_FAILED          0x03
#define RC_NO_UNFINISHED   0x04
#define RC_DAY_STATUS      0x05
#define RC_IN_PROGRESS     0x07

/* ============ CP866 <-> KOI-7 (из genfunc.c терминала) ============
 *
 * На проводе ИПТ использует ?визуальный KOI-7?: кириллица кодируется
 * байтами, визуально похожими на латинские (А?'a', Б?'b', ..., Ь?'x',
 * Ю?'`', Я?'q'). Таблица ? инволюция, работает в обе стороны.
 */
static const uint8_t koi7_map[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0xfd, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x9e, 0x80, 0x81, 0x96, 0x84, 0x85, 0x94, 0x83,
    0x95, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e,
    0x8f, 0x9f, 0x90, 0x91, 0x92, 0x93, 0x86, 0x82,
    0x9c, 0x9b, 0x87, 0x98, 0x9d, 0x99, 0x97, 0x9a,
    0x61, 0x62, 0x77, 0x67, 0x64, 0x65, 0x76, 0x7a,
    0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
    0x72, 0x73, 0x74, 0x75, 0x66, 0x68, 0x63, 0x7e,
    0x7b, 0x7d, 0x7f, 0x79, 0x78, 0x7c, 0x60, 0x71,
    0x61, 0x62, 0x77, 0x67, 0x64, 0x65, 0x76, 0x7a,
    0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x72, 0x73, 0x74, 0x75, 0x66, 0x68, 0x63, 0x7e,
    0x7b, 0x7d, 0x7f, 0x79, 0x78, 0x7c, 0x60, 0x71,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x24, 0x7e, 0x7f,
};

/* CP866 ? KOI-7 (или обратно ? операция инволютивная). */
static void to_koi7(char *s){
    for (size_t i = 0; s[i]; i++)
        s[i] = (char)koi7_map[(uint8_t)s[i]];
}


/* ========================= Byte-order helpers ========================= */

static inline uint16_t rd16(const uint8_t *p){ return (uint16_t)((p[0]<<8)|p[1]); }
static inline uint32_t rd32(const uint8_t *p){
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
static inline void wr16(uint8_t *p, uint16_t v){ p[0]=(uint8_t)(v>>8); p[1]=(uint8_t)v; }
static inline void wr32(uint8_t *p, uint32_t v){
    p[0]=(uint8_t)(v>>24); p[1]=(uint8_t)(v>>16);
    p[2]=(uint8_t)(v>>8);  p[3]=(uint8_t)v;
}

/* ============================== Логирование ============================= */

static int g_verbose = 1;

static void dump_hex(const char *tag, const uint8_t *p, size_t n){
    if (!g_verbose) return;
    fprintf(stderr, "%s (%zu B):\n", tag, n);
    for (size_t i=0;i<n;i+=16){
        fprintf(stderr,"  %04zx: ", i);
        for (size_t j=0;j<16 && i+j<n;j++) fprintf(stderr,"%02X ", p[i+j]);
        fprintf(stderr,"\n");
    }
}

/* ============================ RX (приём) ============================ */

typedef struct {
    int nblocks;
    struct { uint16_t stream; const uint8_t *data; uint16_t len; } blocks[MAX_BLOCKS];
} rx_msg_t;

static int rx_parse(const uint8_t *buf, size_t buflen, rx_msg_t *m){
    if (buflen < 8) return -1;
    if (memcmp(buf,"1000",4)!=0) return -2;
    uint32_t mlen = rd32(buf+4);
    if (mlen + 8 > buflen) return -3;
    if (mlen < 1) return -4;
    const uint8_t *msg = buf+8;
    const uint8_t *end = msg+mlen;
    uint8_t bc = msg[0];
    if (bc > MAX_BLOCKS) return -5;
    m->nblocks = bc;
    const uint8_t *p = msg+1;
    for (int i=0;i<bc;i++){
        if (p+4 > end) return -6;
        m->blocks[i].stream = rd16(p);
        m->blocks[i].len    = rd16(p+2);
        p += 4;
        if (p + m->blocks[i].len > end) return -7;
        m->blocks[i].data = p;
        p += m->blocks[i].len;
    }
    return (int)(8 + mlen);
}

/* ============================ TX (передача) ============================ */

typedef struct {
    int nblocks;
    struct { uint16_t stream; uint8_t data[BLOCK_CAP]; size_t len; } blocks[MAX_BLOCKS];
} tx_msg_t;

static void tx_init(tx_msg_t *m){ m->nblocks = 0; }

static int tx_add_block(tx_msg_t *m, uint16_t stream){
    if (m->nblocks >= MAX_BLOCKS) return -1;
    m->blocks[m->nblocks].stream = stream;
    m->blocks[m->nblocks].len = 0;
    return m->nblocks++;
}
static void tx_put(tx_msg_t *m, int b, const void *d, size_t n){
    if (b<0 || b>=m->nblocks) return;
    if (m->blocks[b].len + n > BLOCK_CAP){ fprintf(stderr,"TX overflow\n"); return; }
    memcpy(m->blocks[b].data + m->blocks[b].len, d, n);
    m->blocks[b].len += n;
}
static void tx_u8 (tx_msg_t *m,int b,uint8_t  v){ tx_put(m,b,&v,1); }
static void tx_u16(tx_msg_t *m,int b,uint16_t v){ uint8_t t[2]; wr16(t,v); tx_put(m,b,t,2); }
static void tx_vb (tx_msg_t *m,int b,const void *p,size_t n){
    tx_u16(m,b,(uint16_t)n);
    if (n) tx_put(m,b,p,n);
}
static void tx_str(tx_msg_t *m,int b,const char *s){ tx_vb(m,b,s,strlen(s)); }

static size_t tx_serialize(const tx_msg_t *m, uint8_t *out, size_t cap){
    size_t body = 1;
    for (int i=0;i<m->nblocks;i++) body += 4 + m->blocks[i].len;
    if (body + 8 > cap) return 0;
    memcpy(out,"1000",4);
    wr32(out+4,(uint32_t)body);
    out[8] = (uint8_t)m->nblocks;
    size_t pos = 9;
    for (int i=0;i<m->nblocks;i++){
        wr16(out+pos, m->blocks[i].stream); pos+=2;
        wr16(out+pos, (uint16_t)m->blocks[i].len); pos+=2;
        memcpy(out+pos, m->blocks[i].data, m->blocks[i].len);
        pos += m->blocks[i].len;
    }
    return pos;
}

/* ============================ PTY ============================ */

static int g_master_fd = -1;
static int g_slave_fd  = -1;

static int setup_pty(const char *link_path){
    int master, slave;
    if (openpty(&master, &slave, NULL, NULL, NULL) < 0){
        perror("openpty");
        return -1;
    }

    struct termios tio;
    if (tcgetattr(slave, &tio) == 0){
        cfmakeraw(&tio);
        tio.c_cc[VMIN]  = 0;
        tio.c_cc[VTIME] = 0;
        tcsetattr(slave, TCSANOW, &tio);
    }
    if (tcgetattr(master, &tio) == 0){
        cfmakeraw(&tio);
        tcsetattr(master, TCSANOW, &tio);
    }

    char *slave_name = ttyname(slave);
    if (!slave_name){
        perror("ttyname");
        close(master); close(slave);
        return -1;
    }

    /* Удаляем предыдущий симлинк, но не трогаем настоящие device-узлы. */
    struct stat st;
    if (lstat(link_path, &st) == 0){
        if (S_ISLNK(st.st_mode)){
            unlink(link_path);
        } else {
            fprintf(stderr,
                "[pty] %s существует и не является симлинком (mode=0%o). "
                "Отказываюсь удалять.\n", link_path, st.st_mode);
            close(master); close(slave);
            return -1;
        }
    }

    if (symlink(slave_name, link_path) < 0){
        perror("symlink");
        close(master); close(slave);
        return -1;
    }

    fprintf(stderr, "[pty] %s -> %s\n", link_path, slave_name);
    g_master_fd = master;
    g_slave_fd  = slave;   /* держим открытым, чтобы pts не исчезал */
    return master;
}

/* ============================ Отправка ============================ */

static int send_msg(const tx_msg_t *m){
    uint8_t buf[262144];
    size_t n = tx_serialize(m, buf, sizeof(buf));
    if (n == 0){ fprintf(stderr,"serialize fail\n"); return -1; }
    dump_hex("TX", buf, n);
    ssize_t w = write(g_master_fd, buf, n);
    if (w < 0){ perror("write master"); return -1; }
    return (int)w;
}
static int send_empty(void){ tx_msg_t m; tx_init(&m); return send_msg(&m); }

static int send_cmd_only(uint8_t cmd){
    tx_msg_t m; tx_init(&m);
    int b = tx_add_block(&m, STREAM_CMD);
    tx_u8(&m, b, cmd);
    return send_msg(&m);
}

/* ============ Билдер ?Потока команд? ============ */

typedef struct {
    tx_msg_t m;
    int      blk;
    uint8_t  n;
} req_builder_t;

static void req_begin(req_builder_t *b, uint8_t cmd){
    tx_init(&b->m);
    b->blk = tx_add_block(&b->m, STREAM_CMD);
    tx_u8(&b->m, b->blk, cmd);
    tx_u8(&b->m, b->blk, 0);   /* N подставим позже */
    b->n = 0;
}
static void req_add(req_builder_t *b, const char *name, bool mandatory){
    tx_str(&b->m, b->blk, name);
    tx_u8 (&b->m, b->blk, mandatory ? 0x01 : 0x00);
    b->n++;
}
static void req_send(req_builder_t *b){
    b->m.blocks[b->blk].data[1] = b->n;
    send_msg(&b->m);
}

/* ============ Экранные хелперы ============ */

static void scr_cls(tx_msg_t *m, int b){ tx_u8(m, b, SCR_CLS); }

static void scr_cur(tx_msg_t *m, int b, uint16_t x, uint16_t y){
    tx_u8(m,b,SCR_CUR); tx_u16(m,b,x); tx_u16(m,b,y);
}

static void scr_print(tx_msg_t *m, int b, const char *s, uint8_t attr){
    char buf[512];
    snprintf(buf, sizeof(buf), "%s", s);
    to_koi7(buf);                        /* CP866 -> KOI-7 */
    tx_u8(m,b,SCR_PRINT);
    tx_vb(m,b,buf,strlen(buf));
    tx_u8(m,b,attr);
}


static void scr_edit(tx_msg_t *m, int b, uint16_t n, const char *name, const char *value){
    char valbuf[128];
    snprintf(valbuf, sizeof(valbuf), "%s", value);
    to_koi7(valbuf);                     /* CP866 -> KOI-7 */
    tx_u8 (m,b,SCR_EDIT);
    tx_u16(m,b,n);
    tx_str(m,b,name);                    /* имя поля ? ASCII */
    tx_vb (m,b,valbuf,strlen(valbuf));
}

static void scr_menu(tx_msg_t *m, int b, uint16_t w, uint16_t h,
        const char *name, const char **items, int n){
    tx_u8 (m,b,SCR_MENU);
    tx_u16(m,b,w);
    tx_u16(m,b,h);
    tx_u8 (m,b,(uint8_t)n);
    /* Имя меню всегда ASCII ("MNU1", "UnfMenu", ...) ? не рекодим. */
    tx_str(m,b,name);
    for (int i=0;i<n;i++){
        char itembuf[64];
        snprintf(itembuf, sizeof(itembuf), "%s", items[i]);
        to_koi7(itembuf);                /* CP866 -> KOI-7 */
        tx_vb(m,b,itembuf,strlen(itembuf));
    }
}

/* ============================ Состояние ============================ */

typedef enum {
    ST_IDLE = 0,                 /* после INIT CHECK или FINISH, ждём INIT */
    ST_INIT_WAIT_MTYPE,          /* послали REQUEST(SUP_FRAG, MTYPE, EBT) */
    ST_INIT_WAIT_MENU_PARAMS,    /* послали REQUEST(<набор>) */
    ST_MENU_ACTIVE,              /* форма/меню показаны, ждём действия */
    ST_FINISH_WAIT_ACK,          /* послали REQUEST(FINISHMENU) ? ждём RESPONSE */
    ST_FINISH_WAIT_RESULTS,      /* послали EMPTY ? ждём REQUEST с результатами */
    ST_FINISH_WAIT_EMPTY_ACK,    /* послали RESPONSE(results) ? ждём EMPTY */
} ipt_state_t;

typedef struct {
    ipt_state_t state;

    char version[32];
    char model[32];
    char serialno[32];
    char osversion[32];
    char tms_id[32];
    char clerk_id[64];
    char term_id[32];
    char famio[64];
    char amount[32];
    char time_[32];
    char order_id[32];
    char ords[1024];
    char rfndinfo[1024];
    char invoice[16];
    char id_pos[16];
    uint8_t edit_flag;

    uint8_t mtype;
    int     day_open;
    int     has_unfinished;

    /* Для FINISH MENU */
    uint8_t pending_result_code;
    char    pending_response_code[4];
} ipt_t;

static ipt_t g_ipt;

/* forward declarations */
static void show_menu_for(uint8_t mtype);
static void action_finish(uint8_t rc, const char *resp_code);

/* ============================ Инициализация ============================ */

static void ipt_init(void){
    memset(&g_ipt, 0, sizeof(g_ipt));
    strncpy(g_ipt.version,  "V.00000126",       sizeof(g_ipt.version)-1);
    strncpy(g_ipt.model,    "IPP320",           sizeof(g_ipt.model)-1);
    strncpy(g_ipt.serialno, "13350PP90091693",  sizeof(g_ipt.serialno)-1);
    strncpy(g_ipt.osversion,"9.32.3 PATCHT",    sizeof(g_ipt.osversion)-1);
    strncpy(g_ipt.tms_id,   "13350PP90091693",  sizeof(g_ipt.tms_id)-1);
    strncpy(g_ipt.clerk_id, "8601D96F083C5674", sizeof(g_ipt.clerk_id)-1);
    strncpy(g_ipt.term_id,  "00000000005934",   sizeof(g_ipt.term_id)-1);
    strncpy(g_ipt.famio,    "АВГУ СТ",          sizeof(g_ipt.famio)-1);
    strncpy(g_ipt.amount,   "0",                sizeof(g_ipt.amount)-1);
    strncpy(g_ipt.time_,    "0",                sizeof(g_ipt.time_)-1);
    strncpy(g_ipt.id_pos,   "VNIIZT01",         sizeof(g_ipt.id_pos)-1);
    g_ipt.day_open = 1;
}

/* =========== Запросы, которые шлёт сам ИПТ =========== */

static void send_req_init(void){
    req_builder_t b;
    req_begin(&b, CMD_REQUEST_PARAMS);
    req_add(&b, "SUPPORT_FRAGMENTATION", false);
    req_add(&b, "MTYPE",                 false);
    req_add(&b, "EBT",                   true);
    req_send(&b);
    g_ipt.state = ST_INIT_WAIT_MTYPE;
}

static void send_req_menu_params(uint8_t mtype){
    req_builder_t b;
    req_begin(&b, CMD_REQUEST_PARAMS);

    switch (mtype){
    case MT_PAY:
        req_add(&b,"CLERKID", true);
        req_add(&b,"TERMID",  true);
        req_add(&b,"TIME",    true);
        req_add(&b,"AMOUNT",  true);
        req_add(&b,"ORDERID", true);
        req_add(&b,"ORDS",    true);
        req_add(&b,"EDIT",    true);
        req_add(&b,"TYPE",    false);
        req_add(&b,"SUBTYPE", false);
        break;
    case MT_RETURN:
        req_add(&b,"CLERKID", true);
        req_add(&b,"TERMID",  true);
        req_add(&b,"TIME",    true);
        req_add(&b,"AMOUNT",  true);
        req_add(&b,"RFNDINFO",false);
        req_add(&b,"EDIT",    true);
        break;
    case MT_CANCEL:
        req_add(&b,"CLERKID", true);
        req_add(&b,"TERMID",  true);
        req_add(&b,"TIME",    true);
        req_add(&b,"AMOUNT",  true);
        req_add(&b,"INVOICE", true);
        req_add(&b,"EDIT",    true);
        break;
    case MT_OPEN_DAY:
    case MT_CLOSE_DAY:
        req_add(&b,"CLERKID", true);
        req_add(&b,"TERMID",  true);
        req_add(&b,"TIME",    true);
        req_add(&b,"FAMIO",   false);
        break;
    case MT_UNFINISHED:
        if (!g_ipt.has_unfinished){
            /* По ТТ: незавершённой операции нет ? сразу возвращаем RC=0x04,
             * NEXT_MTYPE=0xFF, без ожидания кассира. */
            fprintf(stderr,
                "[ipt] MTYPE=0xA0 ? незавершённой операции нет; "
                "автоответ RESULT_CODE=0x04\n");
            /* action_finish требует ST_MENU_ACTIVE ? переводим состояние. */
            g_ipt.state = ST_MENU_ACTIVE;
            action_finish(RC_NO_UNFINISHED, NULL);
            return;
        }
        /* Операция есть ? показываем форму подтверждения ?ПРОДОЛЖИТЬ?. */
        show_menu_for(mtype);
        return;
    case MT_SERVICE:
    default:
        /* Без доп. реквизитов ? сразу показываем меню */
        show_menu_for(mtype);
        return;
    }    
    
    req_send(&b);
    g_ipt.state = ST_INIT_WAIT_MENU_PARAMS;
}

static void send_req_finishmenu(void){
    req_builder_t b;
    req_begin(&b, CMD_REQUEST_PARAMS);
    req_add(&b, "FINISHMENU", true);
    req_send(&b);
    g_ipt.state = ST_FINISH_WAIT_ACK;
}

static void send_response_results(void){
    tx_msg_t m; tx_init(&m);
    int blk = tx_add_block(&m, STREAM_CMD);
    tx_u8(&m, blk, CMD_RESPONSE_PARAMS);
    tx_u8(&m, blk, 0);   /* N позже */
    uint8_t n = 0;

#define PUT(name, vptr, vlen) do {                     \
        tx_str(&m, blk, name);                          \
        tx_vb (&m, blk, (vptr), (vlen));                \
        n++;                                            \
    } while (0)

    uint8_t mt = g_ipt.mtype;
    PUT("MTYPE", &mt, 1);

    uint8_t rc = g_ipt.pending_result_code;
    PUT("RESULT_CODE", &rc, 1);

    if (g_ipt.pending_response_code[0]){
        PUT("RESPONSE_CODE", g_ipt.pending_response_code,
            strlen(g_ipt.pending_response_code));
    }
    PUT("ID_POS", g_ipt.id_pos, strlen(g_ipt.id_pos));

    if (g_ipt.invoice[0]){
        PUT("INVOICE", g_ipt.invoice, strlen(g_ipt.invoice));
    }
    uint8_t nx = 0xFF;
    PUT("NEXT_MTYPE", &nx, 1);

    uint8_t np = 0;
    PUT("NPARAMS", &np, 1);

    m.blocks[blk].data[1] = n;
    send_msg(&m);

#undef PUT
    g_ipt.state = ST_FINISH_WAIT_EMPTY_ACK;
}

/* =============== Хелперы MTYPE / RESPONSE_CODE =============== */

static const char *mtype_name(uint8_t mt){
    switch (mt){
        case MT_UNFINISHED: return "Завершение незавершённой операции";
        case MT_OPEN_DAY:   return "Открытие дня";
        case MT_CLOSE_DAY:  return "Закрытие дня";
        case MT_PAY:        return "Оплата";
        case MT_RETURN:     return "Возврат";
        case MT_CANCEL:     return "Отмена";
        case MT_SERVICE:    return "Сервисные операции";
        default:            return "?";
    }
}

static const char *default_resp_code(uint8_t mtype){
    switch (mtype){
        case MT_OPEN_DAY:
        case MT_CLOSE_DAY:  return "007";
        case MT_PAY:
        case MT_RETURN:
        case MT_CANCEL:     return "001";
        default:            return "000";
    }
}

/* =============== Отображение меню / экранной формы =============== */

static void show_menu_for(uint8_t mtype){
    tx_msg_t m; tx_init(&m);
    int b = tx_add_block(&m, STREAM_SCREEN);

    switch (mtype){
    case MT_PAY:
    case MT_RETURN:
    case MT_CANCEL: {
        char rc_buf[8];
        snprintf(rc_buf, sizeof(rc_buf), "%02X", 0x00);

        scr_cls(&m, b);

        scr_cur(&m, b, 0, 0);
        scr_print(&m, b, mtype_name(mtype), 0x00);

        scr_cur(&m, b, 0, 1);
        if (mtype == MT_CANCEL){
            scr_print(&m, b, "КВИТАНЦИЯ: ", 0x00);
            scr_print(&m, b, g_ipt.invoice, 0x00);
        } else {
            scr_print(&m, b, "СУММА: ", 0x00);
            scr_print(&m, b, g_ipt.amount, 0x00);
            scr_print(&m, b, " РУБ", 0x00);
        }

        scr_cur(&m, b, 0, 3);
        scr_print(&m, b, "RESULT_CODE:", 0x00);
        scr_cur(&m, b, 15, 3);
        scr_edit(&m, b, 2, "RESULT_CODE", rc_buf);

        scr_cur(&m, b, 0, 4);
        scr_print(&m, b, "RESPONSE_CODE:", 0x00);
        scr_cur(&m, b, 15, 4);
        scr_edit(&m, b, 3, "RESPONSE_CODE", default_resp_code(mtype));

        scr_cur(&m, b, 0, 5);
        scr_print(&m, b, "INVOICE:", 0x00);
        scr_cur(&m, b, 15, 5);
        scr_edit(&m, b, 7, "INVOICE", g_ipt.invoice);

        {
            static const char *items[] = { "ОТПРАВИТЬ", "ОТМЕНА" };
            scr_cur(&m, b, 0, 6);          /* было 7 ? теперь 6 */
            scr_menu(&m, b, 11, 2, "MNU1", items, 2);  /* W=11, имя MNU1 */
        }
        
        send_msg(&m);
        g_ipt.state = ST_MENU_ACTIVE;
        fprintf(stderr,
            "[ipt] экранная форма отправлена (MTYPE=0x%02X, %s); "
            "жду нажатия кнопки в терминале.\n",
            mtype, mtype_name(mtype));
        break;
    }

    case MT_OPEN_DAY:
    case MT_CLOSE_DAY: {
        static const char *items[] = { "ОК" };
        scr_cls(&m, b);
        scr_cur(&m, b, 0, 0);
        scr_print(&m, b, mtype == MT_OPEN_DAY ? "ОТКРЫТИЕ ДНЯ" : "ЗАКРЫТИЕ ДНЯ", 0x00);
        scr_cur(&m, b, 0, 2);
        scr_print(&m, b, "КАССИР: ", 0x00);
        scr_print(&m, b, g_ipt.famio, 0x00);
        scr_cur(&m, b, 0, 4);
        scr_menu(&m, b, 12, 1, "DayMenu", items, 1);
        send_msg(&m);
        g_ipt.state = ST_MENU_ACTIVE;
        fprintf(stderr,"[ipt] меню MTYPE=0x%02X (%s) активно; "
                       "команды в консоли: ok / cancel / fail\n",
                mtype, mtype_name(mtype));
        break;
    }

    case MT_SERVICE: {
        static const char *items[] = {
            "БЕЗОПАСНОСТЬ","ЗАГРУЗКА КЛЮЧЕЙ",
            "ПЕЧАТЬ ЖУРНАЛА","ОБНОВЛЕНИЕ TMS","ВЫХОД"
        };
        scr_cls(&m, b);
        scr_cur(&m, b, 0, 0);
        scr_print(&m, b, "AdminMenu", 0x00);
        scr_cur(&m, b, 0, 2);
        scr_menu(&m, b, 18, 6, "AdminMenu", items, 5);
        send_msg(&m);
        g_ipt.state = ST_MENU_ACTIVE;
        fprintf(stderr,"[ipt] меню MTYPE=0x%02X (%s) активно; "
                       "команды в консоли: ok / cancel / fail\n",
                mtype, mtype_name(mtype));
        break;
    }

    case MT_UNFINISHED:
    default: {
        static const char *items[] = { "ПРОДОЛЖИТЬ" };
        scr_cls(&m, b);
        scr_cur(&m, b, 0, 0);
        scr_print(&m, b, "НЕЗАВЕРШЕННАЯ ОПЕРАЦИЯ", 0x04);
        scr_cur(&m, b, 0, 2);
        scr_menu(&m, b, 20, 1, "UnfMenu", items, 1);
        send_msg(&m);
        g_ipt.state = ST_MENU_ACTIVE;
        fprintf(stderr,"[ipt] меню MTYPE=0x%02X (%s) активно; "
                       "команды в консоли: ok / cancel / fail\n",
                mtype, mtype_name(mtype));
        break;
    }
    }
}

/* =============== Разбор REQUEST PARAMETERS от ТМ =============== */

typedef struct { tx_msg_t *resp; int blk; uint8_t n; } resp_ctx_t;

static void cb_add_response(const char *name, size_t nlen, void *user){
    resp_ctx_t *c = user;
    char buf[256]; const char *val = NULL;

    if      (!strncmp(name,"MTYPE",       nlen) && nlen==5 ){ buf[0]=(char)g_ipt.mtype; buf[1]=0; val=buf; }
    else if (!strncmp(name,"EBT",         nlen) && nlen==3 ) val="\x01";
    else if (!strncmp(name,"SUPPORT_FRAGMENTATION", nlen) && nlen==21) val="\x01";
    else if (!strncmp(name,"VERSION",     nlen) && nlen==7 ) val=g_ipt.version;
    else if (!strncmp(name,"CLERKID",     nlen) && nlen==7 ) val=g_ipt.clerk_id;
    else if (!strncmp(name,"TERMID",      nlen) && nlen==6 ) val=g_ipt.term_id;
    else if (!strncmp(name,"TIME",        nlen) && nlen==4 ) val=g_ipt.time_;
    else if (!strncmp(name,"AMOUNT",      nlen) && nlen==6 ) val=g_ipt.amount;
    else if (!strncmp(name,"ORDERID",     nlen) && nlen==7 ) val=g_ipt.order_id;
    else if (!strncmp(name,"ORDS",        nlen) && nlen==4 ) val=g_ipt.ords;
    else if (!strncmp(name,"INVOICE",     nlen) && nlen==7 ) val=g_ipt.invoice;
    else if (!strncmp(name,"FAMIO",       nlen) && nlen==5 ) val=g_ipt.famio;
    else if (!strncmp(name,"MODEL",       nlen) && nlen==5 ) val=g_ipt.model;
    else if (!strncmp(name,"SERIALNO",    nlen) && nlen==8 ) val=g_ipt.serialno;
    else if (!strncmp(name,"OSVERSION",   nlen) && nlen==9 ) val=g_ipt.osversion;
    else if (!strncmp(name,"TMS_ID",      nlen) && nlen==6 ) val=g_ipt.tms_id;
    else if (!strncmp(name,"TYPES",       nlen) && nlen==5 ) val="SBP";
    else if (!strncmp(name,"RFNDINFO",    nlen) && nlen==8 ) val=g_ipt.rfndinfo;
    else if (!strncmp(name,"EDIT",        nlen) && nlen==4 ){ buf[0]=(char)g_ipt.edit_flag; buf[1]=0; val=buf; }
    else return;

    tx_str(c->resp, c->blk, name);
    tx_vb (c->resp, c->blk, val, strlen(val));
    c->n++;
}

static bool params_contain(const uint8_t *arr, size_t alen, const char *want){
    size_t off = 0;
    while (off+2 <= alen){
        uint16_t nl = rd16(arr+off); off+=2;
        if (off+nl+1 > alen) break;
        if (nl==strlen(want) && !memcmp(arr+off, want, nl)) return true;
        off += nl + 1;
    }
    return false;
}

static void on_request_params(const uint8_t *d, uint16_t l){
    if (l < 2) return;
    const uint8_t *arr = d+2;
    size_t alen = l-2;

    /* ТМ запросил у нас результаты сценария FINISH MENU */
    if (params_contain(arr, alen, "RESULT_CODE") &&
        params_contain(arr, alen, "NEXT_MTYPE")){
        send_response_results();
        return;
    }

    /* Обычный REQUEST PARAMETERS ? отвечаем RESPONSE PARAMETERS. */
    tx_msg_t m; tx_init(&m);
    int b = tx_add_block(&m, STREAM_CMD);
    tx_u8(&m, b, CMD_RESPONSE_PARAMS);
    tx_u8(&m, b, 0);

    resp_ctx_t ctx = { &m, b, 0 };
    size_t off = 0;
    while (off+2 <= alen){
        uint16_t nl = rd16(arr+off); off+=2;
        if (off+nl+1 > alen) break;
        char name[64] = {0};
        size_t cp = nl < sizeof(name)-1 ? nl : sizeof(name)-1;
        memcpy(name, arr+off, cp);
        off += nl + 1;   /* пропускаем MAND */
        cb_add_response(name, cp, &ctx);
    }
    m.blocks[b].data[1] = ctx.n;
    send_msg(&m);
}

/* =============== Разбор RESPONSE PARAMETERS от ТМ =============== */

static void on_response_params(const uint8_t *d, uint16_t l){
    if (l < 2) return;
    uint8_t n = d[1];
    const uint8_t *p = d+2;
    size_t rem = l-2;
    bool got_mtype = false;

    for (uint8_t i=0; i<n && rem>2; i++){
        uint16_t nl = rd16(p); p+=2; rem-=2;
        if ((size_t)nl + 2 > rem) break;
        char name[64] = {0};
        size_t cp = nl < sizeof(name)-1 ? nl : sizeof(name)-1;
        memcpy(name, p, cp);
        p += nl; rem -= nl;

        uint16_t vl = rd16(p); p+=2; rem-=2;
        if ((size_t)vl > rem) break;

        if      (!strcmp(name,"MTYPE") && vl>=1){ g_ipt.mtype = p[0]; got_mtype = true; }
        else if (!strcmp(name,"CLERKID")){ size_t k = vl<63?vl:63; memcpy(g_ipt.clerk_id, p, k); g_ipt.clerk_id[k]=0; }
        else if (!strcmp(name,"TERMID")) { size_t k = vl<31?vl:31; memcpy(g_ipt.term_id , p, k); g_ipt.term_id [k]=0; }
        else if (!strcmp(name,"TIME"))   { size_t k = vl<31?vl:31; memcpy(g_ipt.time_   , p, k); g_ipt.time_   [k]=0; }
        else if (!strcmp(name,"AMOUNT")) { size_t k = vl<31?vl:31; memcpy(g_ipt.amount  , p, k); g_ipt.amount  [k]=0; }
        else if (!strcmp(name,"ORDERID")){ size_t k = vl<31?vl:31; memcpy(g_ipt.order_id, p, k); g_ipt.order_id[k]=0; }
        else if (!strcmp(name,"ORDS"))   { size_t k = vl<1023?vl:1023; memcpy(g_ipt.ords, p, k); g_ipt.ords[k]=0; }
        else if (!strcmp(name,"INVOICE")){ size_t k = vl<15?vl:15; memcpy(g_ipt.invoice, p, k); g_ipt.invoice[k]=0; }
        else if (!strcmp(name,"FAMIO"))  { size_t k = vl<63?vl:63; memcpy(g_ipt.famio , p, k); g_ipt.famio [k]=0; }
        else if (!strcmp(name,"RFNDINFO")){ size_t k = vl<1023?vl:1023; memcpy(g_ipt.rfndinfo, p, k); g_ipt.rfndinfo[k]=0; }
        else if (!strcmp(name,"EDIT") && vl>=1) g_ipt.edit_flag = p[0];

        p += vl; rem -= vl;
    }

    /* Реакция на полученный ответ зависит от состояния. */
    if (g_ipt.state == ST_INIT_WAIT_MTYPE){
        if (got_mtype){
            send_req_menu_params(g_ipt.mtype);
        } else {
            /* Реальный ИПТ на RESPONSE без MTYPE шлёт EMPTY (poll),
             * оставаясь в том же состоянии. */
            fprintf(stderr,"[ipt] RESPONSE без MTYPE ? шлём EMPTY, ждём дальше\n");
            send_empty();
        }
    } else if (g_ipt.state == ST_INIT_WAIT_MENU_PARAMS){
        show_menu_for(g_ipt.mtype);
    } else if (g_ipt.state == ST_FINISH_WAIT_ACK){
        /* Пришёл RESPONSE(FINISHMENU=0x01) ? шлём EMPTY,
         * ждём REQUEST с результатами. */
        fprintf(stderr,"[ipt] получен RESPONSE(FINISHMENU) ? шлём EMPTY\n");
        send_empty();
        g_ipt.state = ST_FINISH_WAIT_RESULTS;
    }
}

/* =============== INIT / INIT CHECK =============== */

static void on_init(void){
    fprintf(stderr,"[ipt] INIT\n");
    g_ipt.mtype = 0;
    g_ipt.invoice[0] = 0;
    g_ipt.state = ST_IDLE;
    send_req_init();
}

static void on_init_check(void){
    if (g_ipt.has_unfinished){
        send_cmd_only(CMD_INIT_REQUIRED);
        fprintf(stderr,"[ipt] INIT CHECK -> INIT REQUIRED\n");
    } else {
        send_empty();
        fprintf(stderr,"[ipt] INIT CHECK -> EMPTY\n");
    }
}

static void on_finish(void){
    fprintf(stderr,"[ipt] FINISH\n");
    g_ipt.state = ST_IDLE;
}

/* =============== Клавиатурный поток от ТМ =============== */

static void on_keyboard_input(const uint8_t *d, uint16_t l){
    /* Формат: N(W1) { NAME(VB32), VALUE(VB32) } ... */
    if (l < 2) return;
    uint16_t n = rd16(d);
    const uint8_t *p = d + 2;
    size_t rem = l - 2;

    uint8_t rc      = 0x00;
    char    resp[8] = {0};
    char    inv[16] = {0};
    int     action  = 0;
    bool    got_action = false;

    for (uint16_t i=0; i<n && rem>=2; i++){
        uint16_t nl = rd16(p); p+=2; rem-=2;
        if (nl > rem) break;
        char name[64] = {0};
        size_t cn = nl < 63 ? nl : 63;
        memcpy(name, p, cn);
        p += nl; rem -= nl;

        if (rem < 2) break;
        uint16_t vl = rd16(p); p+=2; rem-=2;
        if (vl > rem) break;
        char val[256] = {0};
        size_t cv = vl < 255 ? vl : 255;
        memcpy(val, p, cv);
        p += vl; rem -= vl;

        if (!strcmp(name,"RESULT_CODE")){
            unsigned v = 0;
            if (sscanf(val,"%x",&v)==1 && v<=0x07) rc = (uint8_t)v;
        } else if (!strcmp(name,"RESPONSE_CODE")){
            strncpy(resp, val, sizeof(resp)-1);
        } else if (!strcmp(name,"INVOICE")){
            strncpy(inv, val, sizeof(inv)-1);
        } else if (!strcmp(name,"MNU1")){
            action = atoi(val);
            got_action = true;
        }
        fprintf(stderr,"[ipt] kbd: %s = \"%s\"\n", name, val);
    }

    if (!got_action){
        /* Простое меню (UnfMenu, DayMenu, AdminMenu): нажатие любой
         * кнопки интерпретируем как ?кассир подтвердил? и отвечаем
         * осмысленными значениями по умолчанию. */
        if (g_ipt.state != ST_MENU_ACTIVE){
            fprintf(stderr,"[ipt] kbd: не в состоянии меню, игнор\n");
            return;
        }
        uint8_t rc = RC_OK;
        const char *resp = NULL;
        switch (g_ipt.mtype){
        case MT_UNFINISHED:
            rc   = RC_NO_UNFINISHED;     /* ?незавершённой операции нет? */
            resp = "000";
            break;
        case MT_OPEN_DAY:
        case MT_CLOSE_DAY:
            rc   = RC_OK;
            resp = "007";                /* административная транзакция OK */
            break;
        case MT_SERVICE:
        default:
            rc   = RC_OK;
            resp = NULL;                 /* без RESPONSE_CODE */
            break;
        }
        fprintf(stderr,
            "[ipt] нажата кнопка простого меню (MTYPE=0x%02X); "
            "автоответ RC=0x%02X, RESPONSE_CODE=\"%s\"\n",
            g_ipt.mtype, rc, resp ? resp : "-");
        action_finish(rc, resp);
        return;
    }
    if (g_ipt.state != ST_MENU_ACTIVE){
        fprintf(stderr,"[ipt] kbd: не в состоянии меню, игнор\n");
        return;
    }

    if (action == 1){                       /* ОТПРАВИТЬ */
        if (inv[0]) strncpy(g_ipt.invoice, inv, sizeof(g_ipt.invoice)-1);
        action_finish(rc, resp[0] ? resp : NULL);
    } else if (action == 2){                /* ОТМЕНА */
        action_finish(RC_ABORTED, NULL);
    } else {
        fprintf(stderr,"[ipt] kbd: неизвестное действие %d\n", action);
    }
}

/* =============== Разбор одного сообщения =============== */

static void handle_message(const rx_msg_t *m){
    if (m->nblocks == 0){
        /* Пришло EMPTY от ТМ. Если мы в состоянии ожидания ack'а
         * после отправки результатов ? посылаем FINISH. Иначе ? echo EMPTY. */
        if (g_ipt.state == ST_FINISH_WAIT_EMPTY_ACK){
            fprintf(stderr,"[ipt] получен EMPTY-ack ? шлём FINISH\n");
            send_cmd_only(CMD_FINISH);
            g_ipt.state = ST_IDLE;
            return;
        }
        send_empty();
        return;
    }

    for (int i=0;i<m->nblocks;i++){
        uint16_t s = m->blocks[i].stream;
        const uint8_t *d = m->blocks[i].data;
        uint16_t l = m->blocks[i].len;

        if (s == STREAM_CMD && l>=1){
            uint8_t cmd = d[0];
            switch (cmd){
                case CMD_INIT:            on_init(); break;
                case CMD_INIT_CHECK:      on_init_check(); break;
                case CMD_REQUEST_PARAMS:  on_request_params(d,l); break;
                case CMD_RESPONSE_PARAMS: on_response_params(d,l); break;
                case CMD_FINISH:          on_finish(); break;
                default:
                    fprintf(stderr,"[ipt] неизвестная команда 0x%02X\n", cmd);
            }
        } else if (s == STREAM_KBD){
            fprintf(stderr,"[ipt] клавиатура: %u байт\n", l);
            on_keyboard_input(d, l);
        } else {
            fprintf(stderr,"[ipt] поток 0x%04X (%u B) проигнорирован\n", s, l);
        }
    }
}

/* =============== ?Действие кассира? ? завершение сценария =============== */

static void action_finish(uint8_t rc, const char *resp_code){
    g_ipt.pending_result_code = rc;
    if (resp_code){
        strncpy(g_ipt.pending_response_code, resp_code, 3);
        g_ipt.pending_response_code[3] = 0;
    } else {
        g_ipt.pending_response_code[0] = 0;
    }
    if (g_ipt.state != ST_MENU_ACTIVE){
        fprintf(stderr,"[ipt] не в меню\n");
        return;
    }
    fprintf(stderr,"[ipt] FINISH MENU: RESULT_CODE=0x%02X, RESPONSE_CODE=\"%s\"\n",
            rc, g_ipt.pending_response_code[0] ? g_ipt.pending_response_code : "-");
    send_req_finishmenu();
}

/* =============== Консоль эмулятора =============== */

static void console_loop(char *line){
    line[strcspn(line,"\r\n")] = 0;
    if (!*line) return;

    if (!strcmp(line,"ok"))          action_finish(RC_OK, "000");
    else if (!strcmp(line,"cancel")) action_finish(RC_ABORTED, NULL);
    else if (!strcmp(line,"fail"))   action_finish(RC_FAILED, "050");
    else if (!strcmp(line,"unfin")){ g_ipt.has_unfinished = 1; fprintf(stderr,"[ipt] has_unfinished=1\n"); }
    else if (!strcmp(line,"fin"))  { g_ipt.has_unfinished = 0; fprintf(stderr,"[ipt] has_unfinished=0\n"); }
    else if (!strcmp(line,"verbose")){ g_verbose = !g_verbose; fprintf(stderr,"[ipt] verbose=%d\n",g_verbose); }
    else if (!strcmp(line,"help")){
        fprintf(stderr,
            "Команды консоли эмулятора:\n"
            "  ok      ? успешное завершение (RESULT_CODE=0x00, RESPONSE_CODE=000)\n"
            "  cancel  ? отмена операции   (RESULT_CODE=0x02)\n"
            "  fail    ? ошибка            (RESULT_CODE=0x03, RESPONSE_CODE=050)\n"
            "  unfin   ? установить флаг незавершённой операции\n"
            "  fin     ? снять флаг\n"
            "  verbose ? вкл/выкл дампы RX/TX\n"
            "  help    ? эта справка\n"
            "Для MTYPE 0xA3/0xA4/0xA5 результат вводится в форме на экране ИПТ.\n");
    }
    else fprintf(stderr,"[ipt] команды: ok|cancel|fail|unfin|fin|verbose|help\n");
}

/* ============================ main ============================ */

static volatile sig_atomic_t g_stop = 0;
static void on_sigint(int s){ (void)s; g_stop = 1; }

int main(int argc, char **argv){
    const char *link_path = (argc > 1) ? argv[1] : DEV_LINK_DEFAULT;

    signal(SIGINT,  on_sigint);
    signal(SIGTERM, on_sigint);

    ipt_init();

    if (setup_pty(link_path) < 0) return 1;

    int fl = fcntl(g_master_fd, F_GETFL, 0);
    fcntl(g_master_fd, F_SETFL, fl | O_NONBLOCK);

    uint8_t *rx = malloc(RX_CAP);
    if (!rx){ perror("malloc"); return 1; }
    size_t rx_len = 0;

    fprintf(stderr,"[ipt] эмулятор запущен. Ждём команды на %s\n", link_path);

    while (!g_stop){
        fd_set rset; FD_ZERO(&rset);
        FD_SET(g_master_fd, &rset);
        FD_SET(STDIN_FILENO, &rset);
        int maxfd = g_master_fd > STDIN_FILENO ? g_master_fd : STDIN_FILENO;

        struct timeval tv = { 1, 0 };
        int ret = select(maxfd+1, &rset, NULL, NULL, &tv);
        if (ret < 0){
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        if (FD_ISSET(g_master_fd, &rset)){
            uint8_t tmp[8192];
            ssize_t n = read(g_master_fd, tmp, sizeof(tmp));
            if (n > 0){
                if (rx_len + (size_t)n > RX_CAP){
                    fprintf(stderr,"[ipt] RX overflow, сбрасываю буфер\n");
                    rx_len = 0;
                } else {
                    memcpy(rx + rx_len, tmp, (size_t)n);
                    rx_len += (size_t)n;
                }
                dump_hex("RX", tmp, (size_t)n);

                while (rx_len >= 8){
                    uint32_t mlen = rd32(rx+4);
                    size_t total = 8 + mlen;
                    if (total > rx_len) break;
                    rx_msg_t msg;
                    int rc = rx_parse(rx, rx_len, &msg);
                    if (rc < 0){
                        fprintf(stderr,"[ipt] parse error %d, сброс\n", rc);
                        rx_len = 0;
                        break;
                    }
                    handle_message(&msg);
                    memmove(rx, rx + total, rx_len - total);
                    rx_len -= total;
                }
            } else if (n == 0){
                fprintf(stderr,"[ipt] PTY: EOF\n");
            } else if (errno == EIO){
                if (g_verbose)
                    fprintf(stderr,"[ipt] PTY: slave закрыт (EIO), ждём переоткрытия\n");
            } else if (errno != EAGAIN && errno != EWOULDBLOCK){
                perror("[ipt] read master");
            }
        }

        if (FD_ISSET(STDIN_FILENO, &rset)){
            char line[256];
            if (fgets(line, sizeof(line), stdin)){
                console_loop(line);
            } else {
                FD_CLR(STDIN_FILENO, &rset);
            }
        }
    }

    free(rx);
    if (g_master_fd >= 0) close(g_master_fd);
    if (g_slave_fd  >= 0) close(g_slave_fd);

    /* Удаляем симлинк, только если он действительно наш. */
    struct stat st;
    if (lstat(link_path, &st) == 0 && S_ISLNK(st.st_mode))
        unlink(link_path);

    return 0;
}