/* Разбор ответов ККТ. (c) gsr 2018-2020 */

#include <assert.h>
#include <string.h>
#include "kkt/io.h"
#include "kkt/parser.h"
#include "x3data/common.h"

uint8_t tx_prefix = KKT_NUL;
uint8_t tx_cmd = KKT_NUL;

uint8_t kkt_status = KKT_STATUS_OK;

static enum {
	st_prefix,
	st_fix_data,
	st_var_data_len,
	st_var_data,
} rx_st = st_prefix;

void begin_rx(void)
{
	kkt_reset_rx();
	kkt_rx_exp_len = (tx_prefix == KKT_NUL) ? 3 : 4;
	rx_st = st_prefix;
}

static inline void begin_fix_data(size_t len)
{
	kkt_rx_exp_len += len;
	rx_st = st_fix_data;
}

static inline void begin_var_data_len(void)
{
	kkt_rx_exp_len += 2;
	rx_st = st_var_data_len;
}

static inline void begin_var_data(size_t len)
{
	kkt_rx_exp_len += len;
	rx_st = st_var_data;
}

static void parse_date(const uint8_t *data, struct kkt_fs_date *date)
{
	date->year = from_bcd(*data++) + 2000;
	date->month = from_bcd(*data++);
	date->day = from_bcd(*data);
}

static void parse_time(const uint8_t *data, struct kkt_fs_time *time)
{
	time->hour = from_bcd(*data++);
	time->minute = from_bcd(*data++);
}

static void parse_date_time(const uint8_t *data, struct kkt_fs_date_time *dt)
{
	parse_date(data, &dt->date);
	parse_time(data + KKT_FS_DATE_LEN, &dt->time);
}

static bool parse_prefix(void)
{
	bool ret = false;
	if ((kkt_rx_len > 1) && (kkt_rx[0] == KKT_ESC2)){
		if (tx_prefix == KKT_NUL){
			if ((kkt_rx_len == 3) && (kkt_rx[1] == tx_cmd)){
				kkt_status = kkt_rx[2];
				ret = true;
			}
		}else if ((kkt_rx_len == 4) && (kkt_rx[1] == tx_prefix) && (kkt_rx[2] == tx_cmd)){
			kkt_status = kkt_rx[3];
			ret = true;
		}
	}
	return ret;
}

static bool parse_status(void)
{
	return parse_prefix();
}

static void parse_var_data_len(struct kkt_var_data *data)
{
	data->len = *(uint16_t *)(kkt_rx + kkt_rx_len - 2);
	if (data->len > 0){
		data->data = kkt_rx + kkt_rx_len;
		begin_var_data(data->len);
	}else
		data->data = NULL;
}

static bool parse_var_data(struct kkt_var_data *data)
{
	assert(data != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret)
			begin_var_data_len();
	}else if (rx_st == st_var_data_len)
		parse_var_data_len(data);
	return ret;
}

static bool parse_fdo_req(struct kkt_fdo_cmd *fdo_cmd)
{
	assert(fdo_cmd != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret){
			fdo_cmd->cmd = kkt_status;
			begin_var_data_len();
		}
	}else if (rx_st == st_var_data_len)
		parse_var_data_len(&fdo_cmd->data);
	return ret;
}

static bool check_rtc_data(const struct kkt_rtc_data *rtc)
{
	bool ret = true;
	static uint8_t mdays[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if ((rtc->year < 2018) || (rtc->month < 1) || (rtc->month > 12) || (rtc->day < 1))
		ret = false;
	else if ((rtc->month == 2) && ((rtc->year % 4) == 0) &&
			(((rtc->year % 100) != 0) || ((rtc->year % 400) == 0)) &&
			(rtc->day > 29))
		ret = false;
	else if (rtc->day > mdays[rtc->month])
		ret = false;
	else if ((rtc->hour > 23) || (rtc->minute > 59) || (rtc->second > 59))
		ret = false;
	return ret;
}

static bool parse_rtc_data(struct kkt_rtc_data *rtc)
{
	assert(rtc != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == RTC_GET_STATUS_OK))
			begin_fix_data(7);
	}else if (rx_st == st_fix_data){
		memset(rtc, 0, sizeof(*rtc));
		if (is_bcd(kkt_rx[4]) && is_bcd(kkt_rx[5]))
			rtc->year = from_bcd(kkt_rx[4]) * 100 + from_bcd(kkt_rx[5]);
		else
			ret = false;
		if (ret && is_bcd(kkt_rx[6]))
			rtc->month = from_bcd(kkt_rx[6]);
		else
			ret = false;
		if (ret && is_bcd(kkt_rx[7]))
			rtc->day = from_bcd(kkt_rx[7]);
		else
			ret = false;
		if (ret && is_bcd(kkt_rx[8]))
			rtc->hour = from_bcd(kkt_rx[8]);
		else
			ret = false;
		if (ret && is_bcd(kkt_rx[9]))
			rtc->minute = from_bcd(kkt_rx[9]);
		else
			ret = false;
		if (ret && is_bcd(kkt_rx[10]))
			rtc->second = from_bcd(kkt_rx[10]);
		else
			ret = false;
		if (ret)
			ret = check_rtc_data(rtc);
	}
	return ret;
}

static bool parse_last_doc_info(struct last_doc_info_arg *arg)
{
	assert(arg != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret){
			if (kkt_status == KKT_STATUS_OK)
				begin_fix_data(12);
			else{
				arg->ldi.last_nr = arg->ldi.last_printed_nr = 0;
				arg->ldi.last_type = arg->ldi.last_printed_type = 0;
				begin_var_data_len();
			}
		}
	}else if (rx_st == st_fix_data){
		off_t offs = kkt_rx_len - 12;
		arg->ldi.last_nr = *(const uint32_t *)(kkt_rx + offs);
		offs += sizeof(uint32_t);
		arg->ldi.last_type = *(const uint16_t *)(kkt_rx + offs);
		offs += sizeof(uint16_t);
		arg->ldi.last_printed_nr = *(const uint32_t *)(kkt_rx + offs);
		offs += sizeof(uint32_t);
		arg->ldi.last_printed_type = *(const uint16_t *)(kkt_rx + offs);
		begin_var_data_len();
	}else if (rx_st == st_var_data_len)
		parse_var_data_len(&arg->err_info);
	return ret;
}

static bool parse_print_last_doc(struct last_printed_info_arg *arg)
{
	assert(arg != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret){
			if (kkt_status == KKT_STATUS_OK)
				begin_fix_data(10);
			else{
				arg->lpi.nr = 0;
				arg->lpi.sign = 0;
				arg->lpi.type = 0;
				begin_var_data_len();
			}
		}
	}else if (rx_st == st_fix_data){
		off_t offs = kkt_rx_len - 10;
		arg->lpi.nr = *(const uint32_t *)(kkt_rx + offs);
		offs += sizeof(uint32_t);
		arg->lpi.sign = *(const uint32_t *)(kkt_rx + offs);
		offs += sizeof(uint32_t);
		arg->lpi.type = *(const uint16_t *)(kkt_rx + offs);
		begin_var_data_len();
	}else if (rx_st == st_var_data_len)
		parse_var_data_len(&arg->err_info);
	return ret;
}

static bool parse_end_doc(struct doc_info_arg *arg)
{
	assert(arg != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret){
			if (kkt_status == KKT_STATUS_OK)
				begin_fix_data(8);
			else{
				arg->di.nr = 0;
				arg->di.sign = 0;
				begin_var_data_len();
			}
		}
	}else if (rx_st == st_fix_data){
		off_t offs = kkt_rx_len - 8;
		arg->di.nr = *(const uint32_t *)(kkt_rx + offs);
		offs += sizeof(uint32_t);
		arg->di.sign = *(const uint32_t *)(kkt_rx + offs);
		begin_var_data_len();
	}else if (rx_st == st_var_data_len)
		parse_var_data_len(&arg->err_info);
	return ret;
}

static bool parse_brightness(struct kkt_brightness *arg)
{
	assert(arg != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret){
			if (kkt_status == KKT_STATUS_OK)
				begin_fix_data(3);
			else
				arg->current = arg->def = arg->max = 0;
		}
	}else if (rx_st == st_fix_data){
		off_t offs = kkt_rx_len - 3;
		arg->current = kkt_rx[offs++];
		arg->def = kkt_rx[offs++];
		arg->max = kkt_rx[offs++];
	}
	return ret;
}

static bool parse_fs_status(struct kkt_fs_status *fs_status)
{
	assert(fs_status != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(30);
	}else if (rx_st == st_fix_data){
		off_t offs = kkt_rx_len - 30;
		fs_status->life_state = kkt_rx[offs++];
		fs_status->current_doc = kkt_rx[offs++];
		fs_status->doc_data = kkt_rx[offs++];
		fs_status->shift_state = kkt_rx[offs++];
		fs_status->alert_flags = kkt_rx[offs++];
		parse_date_time(kkt_rx + offs, &fs_status->dt);
		offs += KKT_FS_DATE_TIME_LEN;
		memcpy(fs_status->nr, kkt_rx + offs, KKT_FS_NR_LEN);
		fs_status->nr[KKT_FS_NR_LEN] = 0;
		offs += KKT_FS_NR_LEN;
		fs_status->last_doc_nr = *(const uint32_t *)(kkt_rx + offs);
	}
	return ret;
}

static bool parse_fs_nr(char *fs_nr)
{
	assert(fs_nr != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(KKT_FS_NR_LEN);
	}else if (rx_st == st_fix_data){
		memcpy(fs_nr, kkt_rx + kkt_rx_len - KKT_FS_NR_LEN, KKT_FS_NR_LEN);
		fs_nr[KKT_FS_NR_LEN] = 0;
	}
	return ret;
}

static bool parse_fs_lifetime(struct kkt_fs_lifetime *lt)
{
	assert(lt != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(7);
	}else if (rx_st == st_fix_data){
		off_t offs = kkt_rx_len - 7;
		parse_date_time(kkt_rx + offs, &lt->dt);
		offs += KKT_FS_DATE_TIME_LEN;
		lt->reg_remain = kkt_rx[offs++];
		lt->reg_complete = kkt_rx[offs];
	}
	return ret;
}

static bool parse_fs_version(struct kkt_fs_version *ver)
{
	assert(ver != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(KKT_FS_VERSION_LEN + 1);
	}else if (rx_st == st_fix_data){
		off_t offs = kkt_rx_len - KKT_FS_VERSION_LEN - 1;
		memcpy(ver->version, kkt_rx + offs, KKT_FS_VERSION_LEN);
		ver->version[KKT_FS_VERSION_LEN] = 0;
		offs += KKT_FS_VERSION_LEN;
		ver->type = kkt_rx[offs];
	}
	return ret;
}

static bool parse_fs_last_error(struct kkt_var_data *err_info)
{
	assert(err_info != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_var_data_len();
	}else if (rx_st == st_var_data_len)
		parse_var_data_len(err_info);
	return ret;
}

static bool parse_fs_shift_state(struct kkt_fs_shift_state *ss)
{
	assert(ss != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(5);
	}else if (rx_st == st_fix_data){
		off_t offs = kkt_rx_len - 5;
		ss->opened = (kkt_rx[offs++] != 0);
		ss->shift_nr = *(const uint16_t *)(kkt_rx + offs);
		offs += sizeof(uint16_t);
		ss->cheque_nr = *(const uint16_t *)(kkt_rx + offs);
	}
	return ret;
}

static bool parse_fs_transmission_state(struct kkt_fs_transmission_state *ts)
{
	assert(ts);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(13);
	}else if (rx_st == st_fix_data){
		off_t offs = kkt_rx_len - 13;
		ts->state = kkt_rx[offs++];
		ts->read_msg_started = (kkt_rx[offs++] != 0);
		ts->sndq_len = *(const uint16_t *)(kkt_rx + offs);
		offs += sizeof(uint16_t);
		ts->first_doc_nr = *(const uint32_t *)(kkt_rx + offs);
		offs += sizeof(uint32_t);
		parse_date_time(kkt_rx + offs, &ts->first_doc_dt);
	}
	return ret;
}

static size_t get_doc_data_len(uint8_t doc_type)
{
	size_t ret = 0;
	switch (doc_type){
		case REGISTRATION:
			ret = sizeof(struct kkt_register_report);
			break;
		case RE_REGISTRATION:
			ret = sizeof(struct kkt_reregister_report);
			break;
		case OPEN_SHIFT:
		case CLOSE_SHIFT:
			ret = sizeof(struct kkt_shift_report);
			break;
		case CALC_REPORT:
			ret = sizeof(struct kkt_calc_report);
			break;
		case CHEQUE:
		case CHEQUE_CORR:
		case BSO:
		case BSO_CORR:
			ret = sizeof(struct kkt_cheque_report);
			break;
		case CLOSE_FS:
			ret = sizeof(struct kkt_close_fs);
			break;
		case OP_COMMIT:
			ret = sizeof(struct kkt_fdo_ack);
			break;
	}
	return ret;
}

static bool parse_fs_find_doc(struct find_doc_info_arg *arg)
{
	assert(arg != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(2);
	}else if (rx_st == st_fix_data){
		arg->fdi.doc_type = kkt_rx[kkt_rx_len - 2];
		arg->fdi.fdo_ack = kkt_rx[kkt_rx_len - 1];
		arg->data.len = get_doc_data_len(arg->fdi.doc_type);
		if (arg->data.len > 0){
			arg->data.data = kkt_rx + kkt_rx_len;
			begin_var_data(arg->data.len);
		}else
			arg->data.data = NULL;
	}
	return ret;
}

static bool parse_fs_fdo_ack(struct kkt_fs_fdo_ack *fdo_ack)
{
	assert(fdo_ack != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(sizeof(struct kkt_fdo_ack));

	}else if (rx_st == st_fix_data){
		off_t offs = kkt_rx_len - sizeof(struct kkt_fdo_ack);
		parse_date_time(kkt_rx + offs, &fdo_ack->dt);
		fdo_ack->dt.date.year = 2000 + kkt_rx[offs++];
		fdo_ack->dt.date.month = kkt_rx[offs++];
		fdo_ack->dt.date.day = kkt_rx[offs++];
		fdo_ack->dt.time.hour = kkt_rx[offs++];
		fdo_ack->dt.time.minute = kkt_rx[offs++];
		memcpy(fdo_ack->fiscal_sign, kkt_rx + offs, sizeof(fdo_ack->fiscal_sign));
		offs += sizeof(fdo_ack->fiscal_sign);
		fdo_ack->doc_nr = *(const uint32_t *)(kkt_rx + offs);
	}
	return ret;
}

static bool parse_fs_unconfirmed_cnt(uint32_t *cnt)
{
	assert(cnt != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(sizeof(uint16_t));
	}else if (rx_st == st_fix_data)
		*cnt = *(const uint16_t *)(kkt_rx + kkt_rx_len - sizeof(uint16_t));
	return ret;
}

static bool parse_fs_doc_stlv_info(struct kkt_fs_doc_stlv_info *di)
{
	assert(di != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(4);
	}else if (rx_st == st_fix_data){
		di->doc_type = *(const uint16_t *)(kkt_rx + kkt_rx_len - 4);
		di->len = *(const uint16_t *)(kkt_rx + kkt_rx_len - 2);
	}
	return ret;
}

static bool parse_fs_doc_tlv(struct read_doc_tlv_arg *arg)
{
	assert(arg != NULL);
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(2);
	}else if (rx_st == st_fix_data){
		arg->tag = *(const uint16_t *)(kkt_rx + kkt_rx_len - 2);
		begin_var_data_len();
	}else if (rx_st == st_var_data_len)
		parse_var_data_len(&arg->data);
	return ret;

}

static bool parse_grid_lst(void)
{
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(1);
	}else if (rx_st == st_fix_data){
		if (kkt_rx_exp_len == 4){
			uint8_t n = kkt_rx[3];
			if ((n > 0) && (n <= SPRN_MAX_GRIDS))
				begin_fix_data(n * sizeof(struct pic_header));
		}
	}
	return ret;
}

static bool parse_icon_lst(void)
{
	bool ret = true;
	if (rx_st == st_prefix){
		ret = parse_prefix();
		if (ret && (kkt_status == KKT_STATUS_OK))
			begin_fix_data(1);
	}else if (rx_st == st_fix_data){
		if (kkt_rx_exp_len == 4){
			uint8_t n = kkt_rx[3];
			if ((n > 0) && (n <= SPRN_MAX_ICONS))
				begin_fix_data(n * sizeof(struct pic_header));
		}
	}
	return ret;
}

parser_t get_parser(uint8_t prefix, uint8_t cmd)
{
	parser_t ret = NULL;
	switch (prefix){
		case KKT_NUL:
			switch (cmd){
				case KKT_VF:
					ret = parse_status;
					break;
				case KKT_GRID_LST:
					ret = parse_grid_lst;
					break;
				case KKT_ICON_LST:
					ret = parse_icon_lst;
					break;
				case KKT_GRID_ERASE_ALL:
				case KKT_ICON_ERASE_ALL:
					ret = parse_status;
					break;
			}
			break;
		case KKT_SRV:
			switch (cmd){
				case KKT_SRV_FDO_IFACE:
				case KKT_SRV_FDO_ADDR:
				case KKT_SRV_FDO_DATA:
				case KKT_SRV_FLOW_CTL:
				case KKT_SRV_RST_TYPE:
				case KKT_SRV_RTC_SET:
				case KKT_SRV_NET_SETTINGS:
				case KKT_SRV_GPRS_SETTINGS:
				case KKT_SRV_SET_BRIGHTNESS:
					ret = parse_status;
					break;
				case KKT_SRV_FDO_REQ:
					ret = parse_fdo_req;
					break;
				case KKT_SRV_RTC_GET:
					ret = parse_rtc_data;
					break;
				case KKT_SRV_LAST_DOC_INFO:
					ret = parse_last_doc_info;
					break;
				case KKT_SRV_PRINT_LAST:
				case KKT_SRV_PRINT_DOC:
					ret = parse_print_last_doc;
					break;
				case KKT_SRV_BEGIN_DOC:
				case KKT_SRV_SEND_DOC:
					ret = parse_var_data;
					break;
				case KKT_SRV_END_DOC:
					ret = parse_end_doc;
					break;
				case KKT_SRV_GET_BRIGHTNESS:
					ret = parse_brightness;
					break;

			}
			break;
		case KKT_FS:
			switch (cmd){
				case KKT_FS_STATUS:
					ret = parse_fs_status;
					break;
				case KKT_FS_NR:
					ret = parse_fs_nr;
					break;
				case KKT_FS_LIFETIME:
					ret = parse_fs_lifetime;
					break;
				case KKT_FS_VERSION:
					ret = parse_fs_version;
					break;
				case KKT_FS_LAST_ERROR:
					ret = parse_fs_last_error;
					break;
				case KKT_FS_SHIFT_PARAMS:
					ret = parse_fs_shift_state;
					break;
				case KKT_FS_TRANSMISSION_STATUS:
					ret = parse_fs_transmission_state;
					break;
				case KKT_FS_FIND_DOC:
					ret = parse_fs_find_doc;
					break;
				case KKT_FS_FIND_FDO_ACK:
					ret = parse_fs_fdo_ack;
					break;
				case KKT_FS_UNCONFIRMED_DOC_CNT:
					ret = parse_fs_unconfirmed_cnt;
					break;
				case KKT_FS_LAST_REG_DATA:
					ret = parse_var_data;
					break;
				case KKT_FS_GET_DOC_STLV:
					ret = parse_fs_doc_stlv_info;
					break;
				case KKT_FS_READ_DOC_TLV:
					ret = parse_fs_doc_tlv;
					break;
				case KKT_FS_RESET:
					ret = parse_status;
					break;
			}
			break;
	}
	return ret;
}
