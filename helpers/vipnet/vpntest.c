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
			VpnNodeInfo *ni = NULL;
			rc = api->getOwnNodeInfo(&ni);
			if (rc.code == 0){
				printf("name = %s; id = 0x%.8x; ip = 0x%.8x; tasksMask = 0x%.8x.\n",
					ni->name, ni->id, ni->ip, ni->tasksMask);
				ret = true;
			}else
				fprintf(stderr, "getOwnNodeInfo %u (%s).\n", rc.code, rc.message);
		}else
			fprintf(stderr, "getVpnStatusError %u (%s).\n", rc.code, rc.message);
	}
	return ret;
}

int main()
{
	bool rc = test_vpn();
	return rc ? 0 : 1;
}
