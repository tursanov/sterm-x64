#include <stdio.h>
#include <vipnet/vpn_api.h>

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

static bool test_vpn(const char *path, const char *psw)
{
	bool ret = false;
	if (vpn_init()){
		ret =	vpn_stop() &&
			vpn_delete_keys() &&
			vpn_install_keys(path, psw) &&
			vpn_start(psw) &&
			vpn_stop();
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
