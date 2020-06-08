#ifndef REF_AGENT_H
#define REF_AGENT_H

#include "list.h"

typedef struct {
	int n;
	char *name;
	char *inn;
	char *description;
	uint8_t pay_agent;
	char *transfer_operator_phone;
	char *pay_agent_operation;
	char *pay_agent_phone;
	char *payment_processor_phone;
	char *money_transfer_operator_name;
	char *money_transfer_operator_address;
	char *money_transfer_operator_inn;
	char *supplier_phone;
} agent_t;

extern list_t agents;

extern int agents_destroy();

#endif // REF_AGENT_H
