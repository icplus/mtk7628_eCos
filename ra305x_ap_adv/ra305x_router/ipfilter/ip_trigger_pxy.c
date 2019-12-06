/*
 * ip_trigger_pxy.c
 * perform port triggering
 */
 

int ippr_trigger_init()
{
	return 0;
}

/*
 * Setup for a new proxy to handle Real Audio.
 */
int ippr_trigger_new(fin, ip, aps, nat)
fr_info_t *fin;
ip_t *ip;
ap_session_t *aps;
nat_t *nat;
{
	ipnat_t *np;
	
	np = nat->nat_ptr;
	
	if ((nat != nat_new(fin, ip, np, NULL, nflags, NAT_INBOUND)))
		printf("can not create inbound port!\n");
}


