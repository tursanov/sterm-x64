#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <inttypes.h>
#include "kkt/fd/ad.h"

/* Вспомогательная функция для вывода отступа */
static void print_indent(FILE *f, int indent) {
    for (int i = 0; i < indent; i++) {
        fputc(' ', f);
    }
}

/* Дамп структуры L */
void L_dump(FILE *f, L *l, int indent) {
    if (!l) {
        print_indent(f, indent);
        fprintf(f, "L: (null)\n");
        return;
    }
    print_indent(f, indent);
    fprintf(f, "L {\n");
    print_indent(f, indent + 2); fprintf(f, "s = %s\n", l->s ? l->s : "(null)");
    print_indent(f, indent + 2); fprintf(f, "p = %u\n", l->p);
    print_indent(f, indent + 2); fprintf(f, "r = %u\n", l->r);
    print_indent(f, indent + 2); fprintf(f, "t = %" PRId64 "\n", l->t);
    print_indent(f, indent + 2); fprintf(f, "n = %u\n", l->n);
    print_indent(f, indent + 2); fprintf(f, "c = %" PRId64 "\n", l->c);
    print_indent(f, indent + 2); fprintf(f, "i = %" PRId64 "\n", l->i);
    print_indent(f, indent + 2); fprintf(f, "h = %s\n", l->h ? l->h : "(null)");
    print_indent(f, indent + 2); fprintf(f, "z = %s\n", l->z ? l->z : "(null)");
    print_indent(f, indent); fprintf(f, "}\n");
}

/* Контекст для обхода списка L */
struct L_dump_ctx {
    FILE *f;
    int indent;
};

/* Callback для list_foreach */
static int L_dump_cb(void *arg, void *obj) {
    struct L_dump_ctx *ctx = (struct L_dump_ctx *)arg;
    L_dump(ctx->f, (L *)obj, ctx->indent);
    return 0; /* продолжить обход */
}

/* Дамп структуры K */
void K_dump(FILE *f, K *k, const char* desc, int indent) {
    if (!k) {
        print_indent(f, indent);
        fprintf(f, "[%s] K: (null)\n", desc);
        return;
    }
    print_indent(f, indent);
    fprintf(f, "[%s] K {\n", desc);

    /* Список составляющих */
    print_indent(f, indent + 2); fprintf(f, "llist: count=%zu\n", k->llist.count);
    struct L_dump_ctx ctx = { f, indent + 4 };
    list_foreach(&k->llist, &ctx, L_dump_cb);

    print_indent(f, indent + 2); fprintf(f, "o = %u\n", k->o);
    print_indent(f, indent + 2); fprintf(f, "a_flag = %s\n", k->a_flag ? "true" : "false");
    print_indent(f, indent + 2); fprintf(f, "a = %" PRId64 "\n", k->a);

    /* doc_no_t поля */
#define PRINT_DOC_NO(name, field) \
    print_indent(f, indent + 2); \
    fprintf(f, #name " = %s\n", k->field.s ? k->field.s : "(null)")

    PRINT_DOC_NO(d,   d);
    PRINT_DOC_NO(r,   r);
    PRINT_DOC_NO(n,   n);
    PRINT_DOC_NO(i1,  i1);
    PRINT_DOC_NO(i2,  i2);
    PRINT_DOC_NO(i21, i21);
    PRINT_DOC_NO(u,   u);
    PRINT_DOC_NO(g,   g);
    PRINT_DOC_NO(b,   b);
#undef PRINT_DOC_NO

    print_indent(f, indent + 2); fprintf(f, "p = %" PRId64 "\n", k->p);
    print_indent(f, indent + 2); fprintf(f, "h = %s\n", k->h ? k->h : "(null)");
    print_indent(f, indent + 2); fprintf(f, "m = %u\n", k->m);
    print_indent(f, indent + 2); fprintf(f, "t = %s\n", k->t ? k->t : "(null)");
    print_indent(f, indent + 2); fprintf(f, "e = %s\n", k->e ? k->e : "(null)");
    print_indent(f, indent + 2); fprintf(f, "z = %s\n", k->z ? k->z : "(null)");
    print_indent(f, indent + 2); fprintf(f, "s = %c (%u)\n", k->s, (unsigned char)k->s);

    /* Информация о банковском абзаце */
    if (k->y) {
        print_indent(f, indent + 2); fprintf(f, "y (bank_info):\n");
        print_indent(f, indent + 4); fprintf(f, "req_id = %" PRIu64 "\n", k->y->req_id);
        print_indent(f, indent + 4); fprintf(f, "term_id = %s\n", k->y->term_id);
        print_indent(f, indent + 4); fprintf(f, "op = %c\n", k->y->op);
        print_indent(f, indent + 4); fprintf(f, "ticket = %s\n", k->y->ticket ? "true" : "false");
        print_indent(f, indent + 4); fprintf(f, "blank_nr = %s\n", k->y->blank_nr);
        print_indent(f, indent + 4); fprintf(f, "repayment = %c\n", k->y->repayment);
        print_indent(f, indent + 4); fprintf(f, "prev_blank_nr = %s\n", k->y->prev_blank_nr);
        print_indent(f, indent + 4); fprintf(f, "amount = %" PRIu64 "\n", k->y->amount);
    } else {
        print_indent(f, indent + 2); fprintf(f, "y = (null)\n");
    }

    print_indent(f, indent + 2); fprintf(f, "i = %s\n", k->i ? "true" : "false");
    print_indent(f, indent + 2); fprintf(f, "in_processing_state = %s\n", k->in_processing_state ? "true" : "false");
    print_indent(f, indent + 2); fprintf(f, "c = %d\n", k->c);
    print_indent(f, indent + 2); fprintf(f, "bank_state = %d\n", k->bank_state);
    print_indent(f, indent + 2); fprintf(f, "print_state = %d\n", k->print_state);
    print_indent(f, indent + 2); fprintf(f, "check_state = %s\n", k->check_state ? "true" : "false");
    print_indent(f, indent + 2); fprintf(f, "v = %u\n", k->v);

    /* Дата и время добавления */
    char timebuf[26];
    struct tm *tm_info = localtime(&k->dt);
    if (tm_info) {
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);
        print_indent(f, indent + 2); fprintf(f, "dt = %s\n", timebuf);
    } else {
        print_indent(f, indent + 2); fprintf(f, "dt = %ld\n", (long)k->dt);
    }

    print_indent(f, indent + 2); fprintf(f, "bank_dt = %s\n", k->bank_dt ? k->bank_dt : "(null)");

    print_indent(f, indent); fprintf(f, "}\n");
    
    fflush(f);
}