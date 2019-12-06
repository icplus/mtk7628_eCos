
#ifndef __PPP_SYS_H__
#define __PPP_SYS_H__

#ifdef __cplusplus
extern "C" {
#endif



/* function declarations */

extern void	output              (PPP_IF_VAR_T *pPppIf, u_char *p, int len);
extern int	read_packet         (PPP_IF_VAR_T *pPppIf, u_char *buf);
extern void	ppp_send_config     (PPP_IF_VAR_T *pPppIf, int mtu, u_long asyncmap, int pcomp,
		                        int accomp);
extern void	ppp_set_xaccm       (PPP_IF_VAR_T *pPppIf, ext_accm accm);
extern void	ppp_recv_config     (PPP_IF_VAR_T *pPppIf, int mru, u_long asyncmap, int pcomp,
		                        int accomp);
extern int	sifvjcomp           (PPP_IF_VAR_T *pPppIf, int vjcomp, int cidcomp, int maxcid);
extern int	sifup               (PPP_IF_VAR_T *pPppIf);
extern int	sifdown             (PPP_IF_VAR_T *pPppIf);
//extern int	sifaddr             (PPP_IF_VAR_T *pPppIf, int o, int h, int m);
extern int	sifaddr             (PPP_IF_VAR_T *pPppIf);
extern int	cifaddr             (PPP_IF_VAR_T *pPppIf, int o, int h);
extern int	cifdefaultroute     (u_long ouraddr);
extern int  sifdns              (PPP_IF_VAR_T *pPppIf);
extern int	get_ether_addr      (PPP_IF_VAR_T *pPppIf, u_long ipaddr, struct sockaddr *hwaddr);
extern int	ppp_available       (void);
extern char	*stringdup          (char *in);


#ifdef __cplusplus
}
#endif

#endif /* __PPP_SYS_H__ */


