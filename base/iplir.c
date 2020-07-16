/* Работа с VipNet Client (Инфотекс). (c) gsr 2003-2020 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vipnet/vpn_api.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "base64.h"
#include "cfg.h"
#include "iplir.h"
#include "paths.h"
#include "tki.h"

bool iplir_disabled = false;		/* работа с VipNet невозможна */

static ItcsVpnApi *api = NULL;

bool iplir_init(void)
{
	api = GetVpnApi();
	return api != NULL;
}

void iplir_release(void)
{
	if (api != NULL){
		iplir_stop();
		api = NULL;
	}
}

static inline bool vpn_ok(const VpnApiReturnCode *rc)
{
	return (rc != NULL) && (rc->code == 0);
}

static inline bool vpn_almost_ok(const VpnApiReturnCode *rc)
{
	return (rc != NULL) && ((rc->code == 0) || ((rc->code & 0x60000) == 0x60000));
}

static void __iplir_show_uninit(const char *fn)
{
	fprintf(stderr, "%s: api == NULL.\n", fn);
}

#define iplir_show_uninit() __iplir_show_uninit(__func__)

static void __iplir_show_err(const char *fn, const char *vpn_fn, const VpnApiReturnCode *rc)
{
	if ((rc != NULL) && (rc->code != 0) && (rc->message != NULL))
		fprintf(stderr, "%s: %s: 0x%x (%s).\n", fn, vpn_fn, rc->code, rc->message);
}

#define iplir_show_err(vpn_fn, rc) __iplir_show_err(__func__, vpn_fn, &rc)

static const char *iplir_get_psw(void)
{
	const char *ret = NULL;
	static uint8_t buf[256];	/* пароль до 144 символов */
	struct stat st;
	if ((stat(IPLIR_PSW_DATA, &st) == 0) && (st.st_size <= sizeof(buf))){
		int fd = open(IPLIR_PSW_DATA, O_RDONLY);
		if (fd != -1){
			ssize_t l = read(fd, buf, st.st_size);
			if (l == st.st_size){
				l = base64_decode(buf, l, buf);
				l = base64_decode(buf, l, buf);
				buf[l] = 0;
				ret = (const char *)buf;
			}
			close(fd);
		}
	}
	return ret;
}

bool iplir_install_keys(void)
{
	bool ret = false;
	if (api != NULL){
		const char *psw = iplir_get_psw();
		if (psw != NULL){
			VpnApiReturnCode rc = api->installKeys(IPLIR_DST, psw);
			if (vpn_almost_ok(&rc))
				ret = true;
			else
				iplir_show_err("installKeys", rc);
		}else
			fprintf(stderr, "%s: ошибка получения пароля ключевого дистрибутива.\n",
				__func__);
	}else
		iplir_show_uninit();
	return ret;
}

bool iplir_delete_keys(void)
{
	bool ret = false;
	if (api != NULL){
		VpnStatus stat;
		VpnApiReturnCode rc = api->getVpnStatus(&stat);
		if (vpn_ok(&rc)){
			if (stat.isKeysInstalled){
				rc = api->deleteKeys();
				if (vpn_ok(&rc))
					ret = true;
				else
					iplir_show_err("deleteKeys", rc);
			}else
				ret = true;
		}else
			iplir_show_err("getVpnStatus", rc);
	}else
		iplir_show_uninit();
	return ret;
}


/* Запуск iplir */
bool iplir_start(void)
{
	bool ret = false;
	if (api != NULL){
		VpnStatus stat;
		VpnApiReturnCode rc = api->getVpnStatus(&stat);
		if (vpn_ok(&rc)){
			if (stat.isVpnEnabled)
				ret = true;
			else{
				const char *psw = iplir_get_psw();
				if (psw != NULL){
					VpnApiReturnCode rc = api->startVpn(psw);
					if (vpn_almost_ok(&rc))
						ret = true;
					else
						iplir_show_err("startVpn", rc);
				}else
					fprintf(stderr, "%s: ошибка получения пароля "
						"ключевого дистрибутива.\n", __func__);
			}
		}else
			iplir_show_err("getVpnStatus", rc);
	}else
		iplir_show_uninit();
	return ret;
}

/* Остановка iplir */
bool iplir_stop(void)
{
	bool ret = false;
	if (api != NULL){
		VpnStatus stat;
		VpnApiReturnCode rc = api->getVpnStatus(&stat);
		if (vpn_ok(&rc)){
			if (stat.isVpnEnabled){
				VpnApiReturnCode rc = api->stopVpn();
				if (vpn_ok(&rc))
					ret = true;
				else
					iplir_show_err("stopVpn", rc);
			}else
				ret = true;
		}else
			iplir_show_err("getVpnStatus", rc);
	}else
		iplir_show_uninit();
	return ret;
}

/* Включено ли шифрование ViPNet */
bool iplir_is_active(void)
{
	bool ret = false;
	if (api != NULL){
		VpnStatus st;
		VpnApiReturnCode rc = api->getVpnStatus(&st);
		if (vpn_ok(&rc))
			ret = st.isKeysInstalled && st.isVpnEnabled;
		else
			iplir_show_err("getVpnStatus", rc);
	}else
		iplir_show_uninit();
	return ret;
}

bool iplir_test(void)
{
	bool ret =
		iplir_init() &&
		iplir_stop() &&
		iplir_delete_keys() &&
		iplir_install_keys() &&
		iplir_start() &&
		iplir_is_active();
	iplir_release();
	return ret;
}
