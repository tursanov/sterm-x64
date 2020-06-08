#include <stdio.h>
#include <vipnet/vpn_api.h>

static bool test_vpn(void)
{
	bool ret = false;
	uint32_t ver = GetVpnApiVersion();
	printf("vpn ver = 0x%.8X\n", ver);
	ItcsVpnApi *api = GetVpnApi();
	if (api == NULL)
		fprintf(stderr, "Error accessing VPN API.\n");
	else{
		VpnStatus stat;
		VpnApiReturnCode rc = api->getVpnStatus(&stat);
		if (rc.code == 0){
			printf("isKeysInstalled = %d; isVpnEnabled = %d.\n",
				stat.isKeysInstalled, stat.isVpnEnabled);
			ret = true;
		}else
			fprintf(stderr, "GetVpnStatusError %u (%s).\n", rc.code, rc.message);
	}
	return ret;
}

int main()
{
	bool rc = test_vpn();
	return rc ? 0 : 1;
}
