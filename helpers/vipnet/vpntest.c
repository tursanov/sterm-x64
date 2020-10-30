#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <vipnet/cipher_api.h>
#include <vipnet/vpn_api.h>
#include <stdio.h>
#include <string.h>

static ItcsVpnApi *vpn_api = NULL;

static bool vpn_init(void)
{
	vpn_api = GetVpnApi();
	if (vpn_api == NULL)
		fprintf(stderr, "Error accessing VPN API.\n");
	else
		printf("VPN API initialized.\n");
	return vpn_api != NULL;
}

static void vpn_release(void)
{
	vpn_api = NULL;
	printf("VPN API released.\n");
}

#if 0
static bool vpn_start(const char *psw)
{
	if (vpn_api == NULL)
		return false;
	bool ret = true;
	VpnApiReturnCode rc = vpn_api->startVpn(psw);
	if (rc.code == 0)
		printf("ViPNet started.\n");
	else if ((rc.code & 0x60000) == 0x60000)
		printf("ViPNet started with warning %x (%s).\n", rc.code, rc.message);
	else{
		fprintf(stderr, "startVpn: %x (%s).\n", rc.code, rc.message);
		ret = false;
	}
	return ret;
}

static bool vpn_stop(void)
{
	if (vpn_api == NULL)
		return false;
	bool ret = true;
	VpnStatus stat;
	VpnApiReturnCode rc = vpn_api->getVpnStatus(&stat);
	if (rc.code == 0){
		if (stat.isVpnEnabled){
			rc = vpn_api->stopVpn();
			if (rc.code != 0){
				fprintf(stderr, "stopVpn: %x (%s).\n", rc.code, rc.message);
				ret = false;
			}
		}
	}else{
		fprintf(stderr, "getVpnStatusError: %x (%s).\n", rc.code, rc.message);
		ret = false;
	}
	if (ret)
		printf("ViPNet stopped.\n");
	return ret;
}

static bool vpn_delete_keys(void)
{
	if (vpn_api == NULL)
		return false;
	bool ret = true;
	VpnStatus stat;
	VpnApiReturnCode rc = vpn_api->getVpnStatus(&stat);
	if (rc.code == 0){
		if (stat.isKeysInstalled){
			rc = vpn_api->deleteKeys();
			if (rc.code == 0)
				printf("ViPNet keys deleted.\n");
			else{
				fprintf(stderr, "deleteKeys: %x (%s).\n", rc.code, rc.message);
				ret = false;
			}
		}else
			printf("ViPNet keys are not installed.\n");
	}else{
		fprintf(stderr, "getVpnStatusError: %x (%s).\n", rc.code, rc.message);
		ret = false;
	}
	return ret;
}

static bool vpn_install_keys(const char *path, const char *psw)
{
	if (vpn_api == NULL)
		return false;
	bool ret = true;
	VpnApiReturnCode rc = vpn_api->installKeys(path, psw);
	if (rc.code == 0)
		printf("ViPNet keys are installed.\n");
	else if ((rc.code & 0x60000) == 0x60000)
		printf("ViPNet keys are installed with warning %x (%s).\n", rc.code, rc.message);
	else
		fprintf(stderr, "installKeys: %x (%s).\n", rc.code, rc.message);
	return ret;
}
#endif

/* Определение количества элементов в массиве */
#define ASIZE(a) (int)((a == NULL) ? 0 : sizeof(a)/sizeof(a[0]))
/* Преобразование беззнакового целого в struct in_addr (например, для inet_ntoa) */
#define dw2ip(dw) (struct in_addr){.s_addr = dw}

static const char *vpn_priv_to_str(enum VpnPrivilege priv)
{
	static struct {
		enum VpnPrivilege priv;
		const char *name;
	} map[] = {
		{VpnPrivilegeUnknown,	"unknown"},
		{VpnPrivilegeMin,	"min"},
		{VpnPrivilegeMiddle,	"medium"},
		{VpnPrivilegeMax,	"max"},
		{VpnPrivilegeSpecial,	"special"},
	};
	const char *ret = NULL;
	for (int i = 0; i < ASIZE(map); i++){
		if (priv == map[i].priv){
			ret = map[i].name;
			break;
		}
	}
	if (ret == NULL){
		static char txt[10];
		snprintf(txt, sizeof(txt), "%d", priv);
		ret = txt;
	}
	return ret;
}

static bool vpn_show_info(void)
{
	bool ret = false;
	static char txt[1024];
	size_t offs = 0;
	int l = 0;
	static char ver[128];
	size_t ver_sz = sizeof(ver);
	const int name_len = 20;
	txt[0] = 0;
	VpnApiReturnCode rc = vpn_api->getClientVersion(ver, &ver_sz);
	if (rc.code == 0){
		l = snprintf(txt + offs, sizeof(txt) - offs, "%-*s%s\n", name_len, "Version", ver);
		if (l > 0)
			offs += l;
		if ((l <= 0) || ((offs + 1) > sizeof(txt)))
			goto ret;
	}else
		fprintf(stderr, "getClientVersion: %x (%s).\n", rc.code, rc.message);
	l = snprintf(txt + offs, sizeof(txt) - offs, "%-*s%d\n", name_len, "VPN API version",
		GetVpnApiVersion());
	if (l > 0)
		offs += l;
	if ((l <= 0) || ((offs + 1) > sizeof(txt)))
		goto ret;
	l = snprintf(txt + offs, sizeof(txt) - offs, "%-*s%d\n", name_len, "Cipher API version",
		GetCipherApiVersion());
	if (l > 0)
		offs += l;
	if ((l <= 0) || ((offs + 1) > sizeof(txt)))
		goto ret;
	VpnStatus vpn_st;
	rc = vpn_api->getVpnStatus(&vpn_st);
	if (rc.code == 0){
		l = snprintf(txt + offs, sizeof(txt) - offs, "%-*s%s\n%-*s%s\n",
			name_len, "Keys", vpn_st.isKeysInstalled ? "installed" : "not installed",
			name_len, "VPN status", vpn_st.isVpnEnabled ? "enabled" : "disabled");
		if (l > 0)
			offs += l;
		if ((l <= 0) || ((offs + 1) > sizeof(txt)))
			goto ret;
	}else
		fprintf(stderr, "getVpnStatus: %x (%s).\n", rc.code, rc.message);
	char *data = NULL;
	uint32_t data_len = 0;
	rc = vpn_api->getClientParam(paramSecurityEncryptionMode,
		strlen(paramSecurityEncryptionMode), &data, &data_len);
	if ((rc.code == 0) && (data != NULL)){
		l = snprintf(txt + offs, sizeof(txt) - offs, "%-*s%s\n",
			name_len, "Encryption mode", data);
		vpn_api->releaseClientParamData(data);
		if (l > 0)
			offs += l;
		if ((l <= 0) || ((offs + 1) > sizeof(txt)))
			goto ret;
	}else
		fprintf(stderr, "getClientParam: %x (%s).\n", rc.code, rc.message);
	enum VpnPrivilege priv = VpnPrivilegeUnknown;
	rc = vpn_api->getOwnPrivilegeLevel(&priv);
	if (rc.code == 0){
		l = snprintf(txt + offs, sizeof(txt) - offs, "%-*s%s\n",
			name_len, "Privilege level", vpn_priv_to_str(priv));
		if (l > 0)
			offs += l;
		if ((l <= 0) || ((offs + 1) > sizeof(txt)))
			goto ret;
	}else
		fprintf(stderr, "getOwnPrivilegeLevel: %x (%s).\n", rc.code, rc.message);
	VpnNodeInfo *node = NULL;
	rc = vpn_api->getOwnNodeInfo(&node);
	if ((rc.code == 0) && (node != NULL)){
		l = snprintf(txt + offs, sizeof(txt) - offs,
				"%-*s%s\n%-*s%.8X\n%-*s%s\n%-*s%.8X\n",
			name_len, "Host name", node->name,
			name_len, "Host ID", node->id,
			name_len, "Host IP", inet_ntoa(dw2ip(htonl(node->ip))),
			name_len, "Host tasks", node->tasksMask);
		vpn_api->releaseVpnNodeInfo(node, 1);
		if (l > 0)
			offs += l;
		if ((l <= 0) || ((offs + 1) > sizeof(txt)))
			goto ret;
	}else
		fprintf(stderr, "getOwnNodeInfo: %x (%s).\n", rc.code, rc.message);
	uint32_t id = 0;
	rc = vpn_api->getActiveCoordinator(&id);
	if (rc.code == 0){
		node = NULL;
		rc = vpn_api->getNodeInfo(id, &node);
		if ((rc.code == 0) && (node != NULL)){
			l = snprintf(txt + offs, sizeof(txt) - offs, "%-*s%.8X, %s, %s, %.8X\n",
				name_len, "Active coordiantor",
				node->id, node->name, inet_ntoa(dw2ip(htonl(node->ip))),
				node->tasksMask);
			vpn_api->releaseVpnNodeInfo(node, 1);
			if (l > 0)
				offs += l;
			if ((l <= 0) || ((offs + 1) > sizeof(txt)))
				goto ret;
		}else
			fprintf(stderr, "getNodeInfo: %x (%s).\n", rc.code, rc.message);
	}else
		fprintf(stderr, "getActiveCoordinator: %x (%s).\n", rc.code, rc.message);
	time_t t = 0;
	rc = vpn_api->getLicenseExpiration(&t);
	if (rc.code == 0){
		struct tm *tm = gmtime(&t);
		if (tm != NULL){
			l = snprintf(txt + offs, sizeof(txt) - offs,
					"%-*s%.2d.%.2d.%.4d %.2d:%.2d:%.2d\n",
				name_len, "License expires on",
				tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
				tm->tm_hour, tm->tm_min, tm->tm_sec);
			if (l > 0)
				offs += l;
			if ((l <= 0) || ((offs + 1) > sizeof(txt)))
				goto ret;
		}else
			fprintf(stderr, "gmtime: %m.\n");
	}else
		fprintf(stderr, "getLicenseExpiration: %x (%s).\n", rc.code, rc.message);
	rc = vpn_api->getClientParam(paramKeyNetworkNumber,
		strlen(paramKeyNetworkNumber), &data, &data_len);
	if ((rc.code == 0) && (data != NULL)){
		l = snprintf(txt + offs, sizeof(txt) - offs, "%-*s%s\n",
			name_len, "Network number", data);
		vpn_api->releaseClientParamData(data);
		if (l > 0)
			offs += l;
		if ((l <= 0) || ((offs + 1) > sizeof(txt)))
			goto ret;
	}else
		fprintf(stderr, "getClientParam: %x (%s).\n", rc.code, rc.message);
	size_t nr = 0;
	node = NULL;
	rc = vpn_api->getNodesInfo(&node, &nr);
	if ((rc.code == 0) && (node != NULL)){
		for (size_t i = 0; i < nr; i++){
			l = snprintf(txt + offs, sizeof(txt) - offs,
				"%-*s%.8X %s %s (%.8X)\n",
			name_len, "Host", node[i].id, node[i].name,
			inet_ntoa(dw2ip(htonl(node[i].ip))), node[i].tasksMask);
			if (l > 0)
				offs += l;
			else
				break;
		}
		vpn_api->releaseVpnNodeInfo(node, nr);
		if ((l <= 0) || ((offs + 1) > sizeof(txt)))
			goto ret;
	}else
		fprintf(stderr, "getNodesInfo: %x (%s).\n", rc.code, rc.message);
	VpnUserInfo *users = NULL;
	rc = vpn_api->getUsersInfo(&users, &nr);
	if ((rc.code == 0) && (users != NULL)){
		for (size_t i = 0; i < nr; i++){
			l = snprintf(txt + offs, sizeof(txt) - offs, "%-*s%.8X (%s)\n",
				name_len, "User", users[i].id, users[i].name);
			if (l > 0)
				offs += l;
			else
				break;
		}
		vpn_api->releaseVpnUserInfo(users, nr);
		if ((l <= 0) || ((offs + 1) > sizeof(txt)))
			goto ret;
	}else
		fprintf(stderr, "getUsersInfo: %x (%s).\n", rc.code, rc.message);
	int32_t log_lvl = 0;
	rc = vpn_api->getLogLevel(&log_lvl);
	if (rc.code == 0){
		l = snprintf(txt + offs, sizeof(txt) - offs, "%-*s%d\n",
			name_len, "Logging level", log_lvl);
		if (l > 0)
			offs += l;
		if ((l <= 0) || ((offs + 1) > sizeof(txt)))
			goto ret;
		ret = true;
	}else
		fprintf(stderr, "getLogLevel: %x (%s).\n", rc.code, rc.message);
	if (ret)
		printf("%s", txt);
ret:
	return ret;
}

static bool test_vpn(const char *path __attribute__((unused)),
	const char *psw __attribute__((unused)))
{
	bool ret = false;
	if (vpn_init()){
		ret =	/*vpn_stop() &&
			vpn_delete_keys() &&
			vpn_install_keys(path, psw) &&
			vpn_start(psw) &&
			vpn_stop();*/
			vpn_show_info();
		vpn_release();
	}
	return ret;
}

int main(int argc, char **argv)
{
	bool rc = false;
	if (argc == 3)
		rc = test_vpn(argv[1], argv[2]);
	else
		fprintf(stderr, "use: vpntest dst-path psw.\n");
	return rc ? 0 : 1;
}
