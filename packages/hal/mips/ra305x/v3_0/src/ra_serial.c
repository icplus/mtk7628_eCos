//=============================================================================
//
//      ra_serial.c
//
//      Simple driver for the 16c550c like serial controllers on the Ra305x board
//
//=============================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under    
// the terms of the GNU General Public License as published by the Free     
// Software Foundation; either version 2 or (at your option) any later      
// version.                                                                 
//
// eCos is distributed in the hope that it will be useful, but WITHOUT      
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License    
// for more details.                                                        
//
// You should have received a copy of the GNU General Public License        
// along with eCos; if not, write to the Free Software Foundation, Inc.,    
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.            
//
// As a special exception, if other files instantiate templates or use      
// macros or inline functions from this file, or you compile this file      
// and link it with other works to produce a work based on this file,       
// this file does not by itself cause the resulting work to be covered by   
// the GNU General Public License. However the source code for this file    
// must still be made available in accordance with section (3) of the GNU   
// General Public License v2.                                               
//
// This exception does not invalidate any other reasons why a work based    
// on this file might be covered by the GNU General Public License.         
// -------------------------------------------                              
// ####ECOSGPLCOPYRIGHTEND####                                              
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):   dmoseley
// Contributors:dmoseley, jskov
// Date:        2001-03-20
// Description: Simple driver for the 16c550c serial controller
//
//####DESCRIPTIONEND####
//
//=============================================================================

#include <pkgconf/hal.h>
#include <pkgconf/system.h>
#include CYGBLD_HAL_PLATFORM_H

#include <cyg/hal/hal_arch.h>           // SAVE/RESTORE GP macros
#include <cyg/hal/hal_io.h>             // IO macros
#include <cyg/hal/hal_if.h>             // interface API
#include <cyg/hal/hal_intr.h>           // HAL_ENABLE/MASK/UNMASK_INTERRUPTS
#include <cyg/hal/hal_misc.h>           // Helper functions
#include <cyg/hal/drv_api.h>            // CYG_ISR_HANDLED


#define RA_SERIAL_BAUD_DIVISOR(_baud)		(HAL_GET_SYSBUS_FREQ()/16/(_baud))
#define CYG_DEV_SERIAL_BAUD_DIVISOR 		RA_SERIAL_BAUD_DIVISOR(CYGNUM_HAL_VIRTUAL_VECTOR_CHANNELS_DEFAULT_BAUD)

#define RA_UART_REG(_base, _reg)		HAL_REG32((_base)+(_reg))


//-----------------------------------------------------------------------------
typedef struct {
    cyg_uint32 base;
    cyg_int32 msec_timeout;
    int isr_vector;
} channel_data_t;


static channel_data_t channels[] = {
    { HAL_RA305X_UARTLITE_BASE, 1000, CYGNUM_HAL_INTERRUPT_UARTL},
    { HAL_RA305X_UART_BASE, 1000, CYGNUM_HAL_INTERRUPT_UART},
#ifdef CONFIG_MT7628_ASIC
	{ HAL_RA305X_UART_LITE3_BASE, 1000, CYGNUM_HAL_INTERRUPT_UARTL3},
#endif
};


//-----------------------------------------------------------------------------
// Set the baud rate

static void cyg_hal_plf_serial_set_baud(cyg_uint32 port, cyg_uint16 baud_divisor)
{
    RA_UART_REG(port, UART_LCR) |= UART_LCR_DLAB;
    RA_UART_REG(port, UART_DLL) = baud_divisor & 0XFF;
    RA_UART_REG(port,UART_DLM)  =( baud_divisor>>8);
    RA_UART_REG(port, UART_LCR) &= ~UART_LCR_DLAB;
}

static void cyg_hal_plf_serial_write(void* __ch_data, const cyg_uint8* __buf, cyg_uint32 __len);

//-----------------------------------------------------------------------------
// The minimal init, get and put functions. All by polling.

void cyg_hal_plf_serial_init_channel(void* __ch_data, int serial_number)
{
    cyg_uint32 port;
	char buf[32] = {0};

    // Some of the diagnostic print code calls through here with no idea what the ch_data is.
    // Go ahead and assume it is channels[0].
    if (__ch_data == 0)
      __ch_data = (void*)&channels[0];

    port = ((channel_data_t*)__ch_data)->base;

    /*  Reset UART controller by generating an one to zero transition */
    if (port == HAL_RA305X_UART_BASE) {
	    HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) = RA305X_SYSCTL_UART_RST;
    } else if (port == HAL_RA305X_UARTLITE_BASE) {
	    HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) = RA305X_SYSCTL_UARTL_RST;
    }
#ifdef CONFIG_MT7628_ASIC
	else if (port == HAL_RA305X_UART_LITE3_BASE)
	    HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) = RA305X_SYSCTL_UARTL3_RST;
#endif 
    HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) = 0;
    
    
    // Disable port interrupts while changing hardware
    RA_UART_REG(port, UART_IER) = 0;

    // Set databits, stopbits and parity.
    RA_UART_REG(port, UART_LCR) = UART_LCR_WLS_8BIT;

    // Set default baud rate.
    cyg_hal_plf_serial_set_baud(port, CYG_DEV_SERIAL_BAUD_DIVISOR);

    // Enable and clear FIFO
    RA_UART_REG(port, UART_FCR) = UART_FCR_FIFOEN | UART_FCR_RXRST | UART_FCR_TXRST | 0x80;
//    RA_UART_REG(port, UART_FCR) = 0;

    // enable RTS to keep host side happy.
    RA_UART_REG(port, UART_MCR) = UART_MCR_RTS | UART_MCR_DTS;

	sprintf(buf, "/dev/ser%d diag init\n\r", serial_number);
	cyg_hal_plf_serial_write(__ch_data, buf, strlen(buf));

    
}


void cyg_hal_plf_serial_putc(void* __ch_data, cyg_uint8 __ch)
{
    cyg_uint32 port;

    // Some of the diagnostic print code calls through here with no idea what the ch_data is.
    // Go ahead and assume it is channels[0].
    if (__ch_data == 0)
      __ch_data = (void*)&channels[0];

    port = ((channel_data_t*)__ch_data)->base;

    CYGARC_HAL_SAVE_GP();

    while (!(RA_UART_REG(port, UART_LSR) & UART_LSR_THRE));

    // Now, the transmit buffer is empty
    RA_UART_REG(port, UART_TBR) = __ch;

    // Hang around until the character has been safely sent.
    while (!(RA_UART_REG(port, UART_LSR) & UART_LSR_THRE));

    CYGARC_HAL_RESTORE_GP();
}


static cyg_bool cyg_hal_plf_serial_getc_nonblock(void* __ch_data, cyg_uint8* ch)
{
    cyg_uint32 port;
    

    // Some of the diagnostic print code calls through here with no idea what the ch_data is.
    // Go ahead and assume it is channels[0].
    if (__ch_data == 0)
      __ch_data = (void*)&channels[0];

    port = ((channel_data_t*)__ch_data)->base;

    if (RA_UART_REG(port, UART_LSR) & UART_LSR_DR) {
	    *ch = RA_UART_REG(port, UART_RBR) & 0xff;
	    return true;
    }
    return false;
}


cyg_uint8 cyg_hal_plf_serial_getc(void* __ch_data)
{
    cyg_uint8 ch;
    CYGARC_HAL_SAVE_GP();

    // Some of the diagnostic print code calls through here with no idea what the ch_data is.
    // Go ahead and assume it is channels[0].
    if (__ch_data == 0)
      __ch_data = (void*)&channels[0];

    while(!cyg_hal_plf_serial_getc_nonblock(__ch_data, &ch));

    CYGARC_HAL_RESTORE_GP();
    return ch;
}


static void cyg_hal_plf_serial_write(void* __ch_data, const cyg_uint8* __buf, cyg_uint32 __len)
{
    CYGARC_HAL_SAVE_GP();

    // Some of the diagnostic print code calls through here with no idea what the ch_data is.
    // Go ahead and assume it is channels[0].
    if (__ch_data == 0)
      __ch_data = (void*)&channels[0];

    while(__len-- > 0)
        cyg_hal_plf_serial_putc(__ch_data, *__buf++);

    CYGARC_HAL_RESTORE_GP();
}


static void cyg_hal_plf_serial_read(void* __ch_data, cyg_uint8* __buf, cyg_uint32 __len)
{
    CYGARC_HAL_SAVE_GP();

    // Some of the diagnostic print code calls through here with no idea what the ch_data is.
    // Go ahead and assume it is channels[0].
    if (__ch_data == 0)
      __ch_data = (void*)&channels[0];

    while(__len-- > 0)
        *__buf++ = cyg_hal_plf_serial_getc(__ch_data);

    CYGARC_HAL_RESTORE_GP();
}


cyg_bool cyg_hal_plf_serial_getc_timeout(void* __ch_data, cyg_uint8* ch)
{
    int delay_count;
    channel_data_t* chan;
    cyg_bool res;
    CYGARC_HAL_SAVE_GP();

    // Some of the diagnostic print code calls through here with no idea what the ch_data is.
    // Go ahead and assume it is channels[0].
    if (__ch_data == 0)
      __ch_data = (void*)&channels[0];

    chan = (channel_data_t*)__ch_data;

    delay_count = chan->msec_timeout * 10; // delay in .1 ms steps

    for(;;) {
        res = cyg_hal_plf_serial_getc_nonblock(__ch_data, ch);
        if (res || 0 == delay_count--)
            break;
        CYGACC_CALL_IF_DELAY_US(100);
    }

    CYGARC_HAL_RESTORE_GP();
    return res;
}


static int cyg_hal_plf_serial_control(void *__ch_data, __comm_control_cmd_t __func, ...)
{
    static int irq_state = 0;
    channel_data_t* chan;
    cyg_uint32 intmask;
    int ret = 0;
    CYGARC_HAL_SAVE_GP();

    // Some of the diagnostic print code calls through here with no idea what the ch_data is.
    // Go ahead and assume it is channels[0].
    if (__ch_data == 0)
      __ch_data = (void*)&channels[0];

    chan = (channel_data_t*)__ch_data;

    switch (__func) {
    case __COMMCTL_IRQ_ENABLE:
        irq_state = 1;

	RA_UART_REG(chan->base, UART_IER) |= UART_IER_ERBFI;
        HAL_INTERRUPT_SET_LEVEL(chan->isr_vector, 1);
        HAL_INTERRUPT_UNMASK(chan->isr_vector);
        break;

    case __COMMCTL_IRQ_DISABLE:
        ret = irq_state;
        irq_state = 0;

	RA_UART_REG(chan->base, UART_IER) &= ~UART_IER_ERBFI;
        HAL_INTERRUPT_MASK(chan->isr_vector);
        break;

    case __COMMCTL_DBG_ISR_VECTOR:
        ret = chan->isr_vector;
        break;

    case __COMMCTL_SET_TIMEOUT:
    {
        va_list ap;

        va_start(ap, __func);

        ret = chan->msec_timeout;
        chan->msec_timeout = va_arg(ap, cyg_uint32);

        va_end(ap);
    }        
    break;

    case __COMMCTL_SETBAUD:
    {
        cyg_uint32 baud_rate;
        cyg_uint16 baud_divisor;
        cyg_uint32 port = chan->base;
        va_list ap;

        va_start(ap, __func);
        baud_rate = va_arg(ap, cyg_uint32);
        va_end(ap);

        baud_divisor = RA_SERIAL_BAUD_DIVISOR(baud_rate);


        // Disable port interrupts while changing hardware
	intmask = RA_UART_REG(port, UART_IER);
	RA_UART_REG(port, UART_IER) = 0;

        // Set baud rate.
        cyg_hal_plf_serial_set_baud(port, baud_divisor);

        // Reenable interrupts if necessary
	RA_UART_REG(port, UART_IER) = intmask;
    }
    break;

    case __COMMCTL_GETBAUD:
        break;

    default:
        break;
    }
    CYGARC_HAL_RESTORE_GP();
    return ret;
}


static int cyg_hal_plf_serial_isr(void *__ch_data, int* __ctrlc, CYG_ADDRWORD vector, CYG_ADDRWORD __data)
{
    int res = 0;
    cyg_uint32 iir;
    cyg_uint8 c;
    channel_data_t* chan;
    CYGARC_HAL_SAVE_GP();

    // Some of the diagnostic print code calls through here with no idea what the ch_data is.
    // Go ahead and assume it is channels[0].
    if (__ch_data == 0)
      __ch_data = (void*)&channels[0];

    chan = (channel_data_t*)__ch_data;

    HAL_INTERRUPT_ACKNOWLEDGE(chan->isr_vector);

    iir = RA_UART_REG(chan->base, UART_IIR) & UART_IIR_ID_MASK;

    *__ctrlc = 0;
    if ((iir == UART_IIR_RX_AVIL) || (iir == UART_IIR_RX_TIMEOUT)) {

        c = RA_UART_REG(chan->base, UART_IIR) & 0xFF;
    
        if( cyg_hal_is_break( &c , 1 ) )
            *__ctrlc = 1;

        res = CYG_ISR_HANDLED;
    }

    CYGARC_HAL_RESTORE_GP();
    return res;
}


static void cyg_hal_plf_serial_init(void)
{
    hal_virtual_comm_table_t* comm;
    int cur = CYGACC_CALL_IF_SET_CONSOLE_COMM(CYGNUM_CALL_IF_SET_COMM_ID_QUERY_CURRENT);

    // Disable interrupts.
    HAL_INTERRUPT_MASK(channels[0].isr_vector);
    HAL_INTERRUPT_MASK(channels[1].isr_vector);
#ifdef CONFIG_MT7628_ASIC
	HAL_INTERRUPT_MASK(channels[2].isr_vector);
#endif

    // Init channels
    cyg_hal_plf_serial_init_channel((void*)&channels[0], 0);
    cyg_hal_plf_serial_init_channel((void*)&channels[1], 1);
#ifdef CONFIG_MT7628_ASIC
	cyg_hal_plf_serial_init_channel((void*)&channels[2], 2);
#endif
    // Setup procs in the vector table

    // Set channel 0
    CYGACC_CALL_IF_SET_CONSOLE_COMM(0);
    comm = CYGACC_CALL_IF_CONSOLE_PROCS();
    CYGACC_COMM_IF_CH_DATA_SET(*comm, &channels[0]);
    CYGACC_COMM_IF_WRITE_SET(*comm, cyg_hal_plf_serial_write);
    CYGACC_COMM_IF_READ_SET(*comm, cyg_hal_plf_serial_read);
    CYGACC_COMM_IF_PUTC_SET(*comm, cyg_hal_plf_serial_putc);
    CYGACC_COMM_IF_GETC_SET(*comm, cyg_hal_plf_serial_getc);
    CYGACC_COMM_IF_CONTROL_SET(*comm, cyg_hal_plf_serial_control);
    CYGACC_COMM_IF_DBG_ISR_SET(*comm, cyg_hal_plf_serial_isr);
    CYGACC_COMM_IF_GETC_TIMEOUT_SET(*comm, cyg_hal_plf_serial_getc_timeout);

   // Set channel 1
    CYGACC_CALL_IF_SET_CONSOLE_COMM(1);
    comm = CYGACC_CALL_IF_CONSOLE_PROCS();
    CYGACC_COMM_IF_CH_DATA_SET(*comm, &channels[1]);
    CYGACC_COMM_IF_WRITE_SET(*comm, cyg_hal_plf_serial_write);
    CYGACC_COMM_IF_READ_SET(*comm, cyg_hal_plf_serial_read);
    CYGACC_COMM_IF_PUTC_SET(*comm, cyg_hal_plf_serial_putc);
    CYGACC_COMM_IF_GETC_SET(*comm, cyg_hal_plf_serial_getc);
    CYGACC_COMM_IF_CONTROL_SET(*comm, cyg_hal_plf_serial_control);
    CYGACC_COMM_IF_DBG_ISR_SET(*comm, cyg_hal_plf_serial_isr);
    CYGACC_COMM_IF_GETC_TIMEOUT_SET(*comm, cyg_hal_plf_serial_getc_timeout);

#ifdef CONFIG_MT7628_ASIC
	// Set channel 2
	 CYGACC_CALL_IF_SET_CONSOLE_COMM(2);
	 comm = CYGACC_CALL_IF_CONSOLE_PROCS();
	 CYGACC_COMM_IF_CH_DATA_SET(*comm, &channels[2]);
	 CYGACC_COMM_IF_WRITE_SET(*comm, cyg_hal_plf_serial_write);
	 CYGACC_COMM_IF_READ_SET(*comm, cyg_hal_plf_serial_read);
	 CYGACC_COMM_IF_PUTC_SET(*comm, cyg_hal_plf_serial_putc);
	 CYGACC_COMM_IF_GETC_SET(*comm, cyg_hal_plf_serial_getc);
	 CYGACC_COMM_IF_CONTROL_SET(*comm, cyg_hal_plf_serial_control);
	 CYGACC_COMM_IF_DBG_ISR_SET(*comm, cyg_hal_plf_serial_isr);
	 CYGACC_COMM_IF_GETC_TIMEOUT_SET(*comm, cyg_hal_plf_serial_getc_timeout);
#endif

    // Restore original console
    CYGACC_CALL_IF_SET_CONSOLE_COMM(cur);
  
}


void cyg_hal_plf_comms_init(void)
{
    static int initialized = 0;

    if (initialized)
        return;

    initialized = 1;

    cyg_hal_plf_serial_init();
}

//-----------------------------------------------------------------------------
// end of ra_serial.c

