#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_if.h>
#include <cyg/hal/plf_io.h>

void sys_reboot(void)
{
    CYGACC_CALL_IF_RESET();
}


