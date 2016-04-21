#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ifaddrs.h>

#define INT_TO_ADDR(_addr) \
		(_addr & 0xFF), \
		(_addr >> 8 & 0xFF), \
		(_addr >> 16 & 0xFF), \
		(_addr >> 24 & 0xFF)

#include "raspberry_pi_revision.h"

char *str_replace(const char *original, const char *pattern, const char *replacement)
{
	size_t const replen = strlen(replacement);
	size_t const patlen = strlen(pattern);
	size_t const orilen = strlen(original);

	size_t patcnt = 0;
	const char * oriptr;
	const char * patloc;

	for (oriptr = original; (patloc = strstr(oriptr, pattern)); (oriptr = (patloc + patlen)))
		patcnt++;

	size_t const retlen = orilen + patcnt * (replen - patlen);
	char * const returned = (char *) malloc( sizeof(char) * (retlen + 1) );

	if (returned != NULL)
	{
		char * retptr = returned;
		for (oriptr = original; (patloc = strstr(oriptr, pattern));
			(oriptr = patloc + patlen))
		{
			size_t const skplen = patloc - oriptr;
			// copy the section until the occurence of the pattern
			strncpy(retptr, oriptr, skplen);
			retptr += skplen;
			// copy the replacement
			strncpy(retptr, replacement, replen);
			retptr += replen;
		}
		// copy the rest of the string.
		strcpy(retptr, oriptr);
	}
	return returned;
}

char** getInterfaceList(int *num)
{
        int i, n;
        char **ret = NULL;
        struct ifaddrs *addrs,*tmp;

        if (num == NULL)
                return NULL;

        if (getifaddrs(&addrs) != 0) {
                perror("getifaddrs");
                return NULL;
        }

        n = 0;
        ret = (char **)malloc( sizeof(char *) );
        for (tmp = addrs; tmp ; tmp = tmp->ifa_next) {
                int found = 0;
		if (strcmp(tmp->ifa_name, "lo") == 0)
			found = 1;
		else {
	               for (i = 0; i < n; i++) {
        	               if (strcmp(tmp->ifa_name, ret[i]) == 0) {
                	               found = 1;
                        	       break;
	                       }
        	       }
		}

                if (found == 0) {
                        ret = realloc( ret, (n + 1) * sizeof(char *) );
                        ret[n++] = strdup(tmp->ifa_name);
                }
        }

        *num = n;
        freeifaddrs(addrs);
        return ret;
}

int getInterfaces(char *fmt, int all)
{       
	char tmp[32] = { 0 };
	unsigned char mac_address[6] = { 0 };
	int addr = 0, baddr = 0, mask = 0, network = 0;
        int i, sock, num;
	struct ifreq ifr;

	char **iList = getInterfaceList(&num);
	if ((iList == NULL) || (num == 0))
		return 1;

	sock = socket(PF_INET, SOCK_DGRAM, 0);

	for (i = 0; i < num; i++) {
		addr = 0;
		baddr = 0;
		mask = 0;

		memset(mac_address, 0, sizeof(mac_address));

		strncpy(ifr.ifr_name, iList[i], IFNAMSIZ-1);
		if (ioctl(sock, SIOCGIFADDR, &ifr) == 0)
			addr = ((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr.s_addr;

		if (ioctl(sock, SIOCGIFBRDADDR, &ifr) == 0)
			baddr = ((struct sockaddr_in *)(&ifr.ifr_broadaddr))->sin_addr.s_addr;

		if (ioctl(sock, SIOCGIFNETMASK, &ifr) == 0)
			mask = ((struct sockaddr_in *)(&ifr.ifr_netmask))->sin_addr.s_addr;

		if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0)
			memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);

		if ((all) || (addr > 0)) {
			char *format = strdup( fmt );
			format = str_replace(format, "{DEV}", ifr.ifr_name);
			if (addr > 0)
				snprintf(tmp, sizeof(tmp), "%d.%d.%d.%d", INT_TO_ADDR(addr));
			else
				snprintf(tmp, sizeof(tmp), "N/A");
			format = str_replace(format, "{IPADDR}", tmp);

			int j;
			char tmpX[4] = { 0 };
			memset(tmp, 0, sizeof(tmp));
			for (j = 0; j < 6; j++) {
				snprintf(tmpX, sizeof(tmpX), "%02X:", mac_address[j]);
				strcat(tmp, tmpX);
			}
			if (strlen(tmp) > 0) {
				tmp[strlen(tmp) - 1] = 0;
				format = str_replace(format, "{HWADDR}", tmp);
			}

			memset(tmp, 0, sizeof(tmp));
			for (j = 0; j < 6; j++) {
				snprintf(tmpX, sizeof(tmpX), "%02x:", mac_address[j]);
				strcat(tmp, tmpX);
			}
			if (strlen(tmp) > 0) {
				tmp[strlen(tmp) - 1] = 0;
				format = str_replace(format, "{HWAddr}", tmp);
			}

			if (baddr > 0)
				snprintf(tmp, sizeof(tmp), "%d.%d.%d.%d", INT_TO_ADDR(baddr));
			else
				snprintf(tmp, sizeof(tmp), "N/A");

			format = str_replace(format, "{BRDADDR}", tmp);

			if (mask > 0)
				snprintf(tmp, sizeof(tmp), "%d.%d.%d.%d", INT_TO_ADDR(mask));
			else
				snprintf(tmp, sizeof(tmp), "N/A");

			format = str_replace(format, "{NETMASK}", tmp);
			network = addr & mask;

			if (network > 0)
				snprintf(tmp, sizeof(tmp), "%d.%d.%d.%d", INT_TO_ADDR(network));
			else
				snprintf(tmp, sizeof(tmp), "N/A");

			format = str_replace(format, "{NETWORK}", tmp);

			format = str_replace(format, "\\n", "\n");

			printf("%s", format);
			free(format);
		}

		free(iList[i]);
        }
	free(iList);
        close(sock);

        return i;
}

int main(int argc, char *argv[])
{
	RASPBERRY_PI_INFO_T info;

	if ((argc == 3) && (strcmp(argv[1], "--format") == 0)) {
		char tmp[8] = { 0 };
		char *format = strdup( argv[2] );

		if (getRaspberryPiInformation(&info) > 0) {
			snprintf(tmp, sizeof(tmp), "%d", info.pcbRevision);
			format = str_replace(format, "{MANUFACTURER}", raspberryPiManufacturerToString(info.manufacturer));
			format = str_replace(format, "{MODEL}", raspberryPiModelToString(info.model));
			format = str_replace(format, "{PROCESSOR}", raspberryPiProcessorToString(info.processor));
			format = str_replace(format, "{MEMORY}", raspberryPiMemoryToString(info.memory));
			format = str_replace(format, "{I2CDEV}", raspberryPiI2CDeviceToString(info.i2cDevice));
			format = str_replace(format, "{PCBREV}", tmp);
			snprintf(tmp, sizeof(tmp), "%04x", info.revisionNumber);
			format = str_replace(format, "{REVNO}", tmp);
			format = str_replace(format, "\\n", "\n");

			printf("%s", format);
			free(format);
		}
		else
			fprintf(stderr, "Error: Cannot get information\n");

		return 0;
	}
	else
	if ((argc == 3) && (strncmp(argv[1], "--format-network", 16) == 0)) {
		getInterfaces(argv[2], (strcmp(argv[1], "--format-network-all") == 0) );
		return 0;
	}
	else
	if ((argc == 2) && (strcmp(argv[1], "--show") == 0)) {
		if (getRaspberryPiInformation(&info) > 0) {
			printf("memory: %s\n", raspberryPiMemoryToString(info.memory));
			printf("processor: %s\n",
				raspberryPiProcessorToString(info.processor));
			printf("i2cDevice: %s\n",
				raspberryPiI2CDeviceToString(info.i2cDevice));
			printf("model: %s\n",
				raspberryPiModelToString(info.model));
			printf("manufacturer: %s\n",
				raspberryPiManufacturerToString(info.manufacturer));
			printf("PCB revision: %d\n", info.pcbRevision);
			printf("warranty void: %s\n", (info.warrantyBit) ? "yes" : "no");
			printf("revision: %04x\n", info.revisionNumber);
			printf("peripheral base: 0x%"PRIX32"\n", info.peripheralBase);
			printf("\n");
		}
		else {
			printf("getRaspberryPiInformation() failed ...\n");
		}
	}
	else {
		printf("Syntax: %s [--show|--format-network{|-all} <fmtNic>|--format <fmtPi>\n", argv[0]);
		printf("<fmtPi> can contain \\n, {MANUFACTURER}, {MODEL}, {PROCESSOR}, {MEMORY}, {I2CDEV}, {PCBREV} or {REVNO}\n");
		printf("<fmtNic> can contain \\n, {DEV}, {IPADDR}, {HWADDR}, {HWAddr}, {BRDADDR}, {NETMASK} or {NETWORK}\n");
	}

	return 0;
}
