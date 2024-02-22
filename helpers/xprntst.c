#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sys/ioctls.h"

static bool test_xprn(const char *path)
{
	bool ret = false;
	if (path == NULL)
		return ret;
	int xprn = open(path, O_RDWR);
	if (xprn == -1)
		fprintf(stderr, "%s: error opening %s for read/write: %m.\n", __func__, path);
	else{
		int rc = ioctl(xprn, XPRN_IO_RESET);
		if (rc == 0){
			rc = ioctl(xprn, XPRN_IO_OUTCHAR, 0x41);
			if (rc == 0)
				ret = true;
			else if (rc > 0)
				fprintf(stderr, "%s: XPRN_IO_OUTCHAR error: %d.\n", __func__, rc);
			else
				fprintf(stderr, "%s: XPRN_IO_OUTCHAR error: %m (rc = %d).\n",
					__func__, rc);
		}else
			fprintf(stderr, "%s: XPRN_IO_RESET error: %m.\n", __func__);
		rc = close(xprn);
		if (rc != 0)
			fprintf(stderr, "%s: error closing %s: %m.\n", __func__, path);
	}
	if (ret)
		puts("xprn test complete.");
	return ret;
}

int main()
{
	return test_xprn(XPRN_DEV_NAME) ? 0 : 1;
}
