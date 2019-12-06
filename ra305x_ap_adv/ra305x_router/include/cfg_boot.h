#ifndef CFG_BOOT_H
#define CFG_BOOT_H

typedef	struct tagBootParm
{
    unsigned int version;
    char version1[15-4];
    unsigned char vlan;
    unsigned char my_mac[6];
    unsigned char my_mac2[6];

    char fname[32];
    unsigned int my_ip;
    unsigned int net_mask;
    unsigned int gw_ip;
    unsigned int server_ip;
    unsigned int timeout;
    int intf;               // boot ether i/f 
    int app;                // boot action ,  load 1. linux 2. eboot 3. boot from net 4. run ram image 
    unsigned int load_addr; // default download buffer 
	unsigned int serial;			// serial number
    unsigned int hw_ver;			// h/w version

} tBootParm;

#endif /* CFG_BOOT_H */


