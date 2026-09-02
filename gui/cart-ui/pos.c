#include <stdlib.h>
#include "pos/command.h"
#include "gui/fa.h"

static struct pos_response* run_pos_operation(uint8_t mtype)
{
    struct pos_query_params params =
    {
        .amount = 0,
        .order_id = 0,
        .can_edit = false,
        .time = time(NULL),
        .ords = NULL,
        .type = NULL,
        .subtype = NULL,
        .famio = cashier_get_name_alt(),
        .rfnd_info = NULL,
        .mtype = mtype
    };

    return pos_query(&params);
}

void pos_check_last_operation()
{
    /*struct pos_response* resp = */run_pos_operation(0xa0);
}

void pos_day_open()
{
    /*struct pos_response* resp = */run_pos_operation(0xa1);
}

void pos_day_close()
{
    /*struct pos_response* resp = */run_pos_operation(0xa2);
}

void pos_service_operations()
{
    /*struct pos_response* resp = */run_pos_operation(0xa6);
}
