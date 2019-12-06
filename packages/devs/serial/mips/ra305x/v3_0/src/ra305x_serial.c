//==========================================================================
//
//      ra305x_serial.c
//
//      Serial device driver for RA305X on-chip serial devices
//
//==========================================================================
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
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):   dmoseley, based on POWERPC driver by jskov
// Contributors: gthomas, jskov, dmoseley
// Date:        2000-06-23
// Purpose:     Ralink Ra305x serial device driver
// Description: Ralink Ra305x serial device driver
//
// To Do:
//   Put in magic to effectively use the FIFOs. Transmitter FIFO fill is a
//   problem, and setting receiver FIFO interrupts to happen only after
//   n chars may conflict with hal diag.
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/io_serial.h>
#include <pkgconf/io.h>

#include <cyg/io/io.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/io/devtab.h>
#include <cyg/infra/diag.h>
#include <cyg/io/serial.h>

#ifdef CYGPKG_IO_SERIAL_MIPS_RA305X


typedef struct ra305x_serial_info_s {
    CYG_ADDRWORD   base;
    CYG_WORD       int_num;
    cyg_interrupt  serial_interrupt;
    cyg_handle_t   serial_interrupt_handle;
    cyg_uint8      iir;
} ra305x_serial_info_t;



#define RA305X_UARTREG(_chan, _reg)		HAL_REG32((_chan)->base+(_reg))
#define RA_SERIAL_BAUD_DIVISOR(_baud)		(CYGPKG_IO_SERIAL_MIPS_RA305X_SYSBUS_FREQ/16/(_baud))

static unsigned long select_baud[] = {
    0,    	// Unused
    0,    	// 50
    0,    	// 75
    0, 		// 110
    0,    	// 134.5
    0, 		// 150
    0,    	// 200
    0,  	// 300
    0,  	// 600
    600,  	// 1200
    1800,    	// 1800
    2400,   	// 2400
    3600,    	// 3600
    4800,   	// 4800
    7200,   	// 7200
    9600,   	// 9600
    14400,   	// 14400
    19200,   	// 19200
    38400,    	// 38400
    57600,    	// 57600
    115200,    	// 115200
    230400,	// 230400
    460800,	// 460800
    0,		// 921600
};

static long select_word_length[] = {
    -1,    // 5 bits / word (char)
    UART_LCR_WLS_6BIT,
    UART_LCR_WLS_7BIT,
    UART_LCR_WLS_8BIT
};

static long select_stop_bits[] = {
    -1,
    UART_LCR_1STB,    	// 1 stop bit
    UART_LCR_2STB,  	// 1.5 stop bit
    UART_LCR_2STB     	// 2 stop bits
};


static long select_parity[] = {
    0,     			// No parity
    UART_LCR_PEN|UART_LCR_EPS,  // Even parity
    UART_LCR_PEN,     		// Odd parity
    -1,     // Mark parity
    -1,     // Space parity
};


static bool ra305x_serial_init(struct cyg_devtab_entry *tab);
static bool ra305x_serial_putc(serial_channel *chan, unsigned char c);
static Cyg_ErrNo ra305x_serial_lookup(struct cyg_devtab_entry **tab, 
                                   struct cyg_devtab_entry *sub_tab,
                                   const char *name);
static unsigned char ra305x_serial_getc(serial_channel *chan);
static Cyg_ErrNo ra305x_serial_set_config(serial_channel *chan, cyg_uint32 key,
                                         const void *xbuf, cyg_uint32 *len);
static void ra305x_serial_start_xmit(serial_channel *chan);
static void ra305x_serial_stop_xmit(serial_channel *chan);

static cyg_uint32 ra305x_serial_isr(cyg_vector_t vector, cyg_addrword_t data);
static void       ra305x_serial_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data);

static SERIAL_FUNS(ra305x_serial_funs, 
                   ra305x_serial_putc, 
                   ra305x_serial_getc,
                   ra305x_serial_set_config,
                   ra305x_serial_start_xmit,
                   ra305x_serial_stop_xmit
    );


#if CYGPKG_IO_SERIAL_MIPS_RA305X_SERIAL_0
static ra305x_serial_info_t ra305x_serial_info0 ={base: HAL_RA305X_UARTLITE_BASE,  
						int_num: CYGNUM_HAL_INTERRUPT_UARTL};

#if CYGNUM_IO_SERIAL_MIPS_RA305X_SERIAL_0_BUFSIZE > 0
static unsigned char ra305x_serial_out_buf0[CYGNUM_IO_SERIAL_MIPS_RA305X_SERIAL_0_BUFSIZE];
static unsigned char ra305x_serial_in_buf0[CYGNUM_IO_SERIAL_MIPS_RA305X_SERIAL_0_BUFSIZE];

static SERIAL_CHANNEL_USING_INTERRUPTS(ra305x_serial_channel0,
                                       ra305x_serial_funs, 
                                       ra305x_serial_info0,
                                       CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_MIPS_RA305X_SERIAL_0_BAUD),
                                       CYG_SERIAL_STOP_DEFAULT,
                                       CYG_SERIAL_PARITY_DEFAULT,
                                       CYG_SERIAL_WORD_LENGTH_DEFAULT,
                                       CYG_SERIAL_FLAGS_DEFAULT,
                                       &ra305x_serial_out_buf0[0], 
                                       sizeof(ra305x_serial_out_buf0),
                                       &ra305x_serial_in_buf0[0], 
                                       sizeof(ra305x_serial_in_buf0)
    );
#else
static SERIAL_CHANNEL(ra305x_serial_channel0,
                      ra305x_serial_funs, 
                      ra305x_serial_info0,
                      CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_MIPS_RA305X_SERIAL_0_BAUD),
                      CYG_SERIAL_STOP_DEFAULT,
                      CYG_SERIAL_PARITY_DEFAULT,
                      CYG_SERIAL_WORD_LENGTH_DEFAULT,
                      CYG_SERIAL_FLAGS_DEFAULT
    );
#endif

DEVTAB_ENTRY(ra305x_serial_io0, 
             CYGDAT_IO_SERIAL_MIPS_RA305X_SERIAL_0_NAME,
             0,                 // Does not depend on a lower level interface
             &cyg_io_serial_devio, 
             ra305x_serial_init, 
             ra305x_serial_lookup,     // Serial driver may need initializing
             &ra305x_serial_channel0
    );
    
#endif // !!CYGPKG_IO_SERIAL_MIPS_RA305X_SERIAL0


#if CYGPKG_IO_SERIAL_MIPS_RA305X_SERIAL_1
static ra305x_serial_info_t ra305x_serial_info1 = {base: HAL_RA305X_UART_BASE,  
						int_num: CYGNUM_HAL_INTERRUPT_UART};

#if CYGNUM_IO_SERIAL_MIPS_RA305X_SERIAL_1_BUFSIZE > 0
static unsigned char ra305x_serial_out_buf1[CYGNUM_IO_SERIAL_MIPS_RA305X_SERIAL_1_BUFSIZE];
static unsigned char ra305x_serial_in_buf1[CYGNUM_IO_SERIAL_MIPS_RA305X_SERIAL_1_BUFSIZE];

static SERIAL_CHANNEL_USING_INTERRUPTS(ra305x_serial_channel1,
                                       ra305x_serial_funs, 
                                       ra305x_serial_info1,
                                       CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_MIPS_RA305X_SERIAL_1_BAUD),
                                       CYG_SERIAL_STOP_DEFAULT,
                                       CYG_SERIAL_PARITY_DEFAULT,
                                       CYG_SERIAL_WORD_LENGTH_DEFAULT,
                                       CYG_SERIAL_FLAGS_DEFAULT,
                                       &ra305x_serial_out_buf1[0], 
                                       sizeof(ra305x_serial_out_buf1),
                                       &ra305x_serial_in_buf1[0], 
                                       sizeof(ra305x_serial_in_buf1)
    );
#else
static SERIAL_CHANNEL(ra305x_serial_channel1,
                      ra305x_serial_funs, 
                      ra305x_serial_info1,
                      CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_MIPS_RA305X_SERIAL_1_BAUD),
                      CYG_SERIAL_STOP_DEFAULT,
                      CYG_SERIAL_PARITY_DEFAULT,
                      CYG_SERIAL_WORD_LENGTH_DEFAULT,
                      CYG_SERIAL_FLAGS_DEFAULT
    );
#endif

DEVTAB_ENTRY(ra305x_serial_io1, 
             CYGDAT_IO_SERIAL_MIPS_RA305X_SERIAL_1_NAME,
             0,                 // Does not depend on a lower level interface
             &cyg_io_serial_devio, 
             ra305x_serial_init, 
             ra305x_serial_lookup,     // Serial driver may need initializing
             &ra305x_serial_channel1
    );
    
#endif // !!CYGPKG_IO_SERIAL_MIPS_RA305X_SERIAL_1

#ifdef CONFIG_MT7628_ASIC // CYGPKG_IO_SERIAL_MIPS_RA305X_SERIAL_1
static ra305x_serial_info_t ra305x_serial_info2 = {base: HAL_RA305X_UART_LITE3_BASE,  
						int_num: CYGNUM_HAL_INTERRUPT_UARTL3};

#if CYGNUM_IO_SERIAL_MIPS_RA305X_SERIAL_1_BUFSIZE > 0
static unsigned char ra305x_serial_out_buf2[CYGNUM_IO_SERIAL_MIPS_RA305X_SERIAL_1_BUFSIZE];
static unsigned char ra305x_serial_in_buf2[CYGNUM_IO_SERIAL_MIPS_RA305X_SERIAL_1_BUFSIZE];

static SERIAL_CHANNEL_USING_INTERRUPTS(ra305x_serial_channel2,
                                       ra305x_serial_funs, 
                                       ra305x_serial_info2,
                                       CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_MIPS_RA305X_SERIAL_1_BAUD),
                                       CYG_SERIAL_STOP_DEFAULT,
                                       CYG_SERIAL_PARITY_DEFAULT,
                                       CYG_SERIAL_WORD_LENGTH_DEFAULT,
                                       CYG_SERIAL_FLAGS_DEFAULT,
                                       &ra305x_serial_out_buf2[0], 
                                       sizeof(ra305x_serial_out_buf2),
                                       &ra305x_serial_in_buf2[0], 
                                       sizeof(ra305x_serial_in_buf2)
    );
#else
static SERIAL_CHANNEL(ra305x_serial_channel2,
                      ra305x_serial_funs, 
                      ra305x_serial_info2,
                      CYG_SERIAL_BAUD_RATE(CYGNUM_IO_SERIAL_MIPS_RA305X_SERIAL_1_BAUD),
                      CYG_SERIAL_STOP_DEFAULT,
                      CYG_SERIAL_PARITY_DEFAULT,
                      CYG_SERIAL_WORD_LENGTH_DEFAULT,
                      CYG_SERIAL_FLAGS_DEFAULT
    );
#endif

DEVTAB_ENTRY(ra305x_serial_io2, 
             "/dev/ser2",
             0,                 // Does not depend on a lower level interface
             &cyg_io_serial_devio, 
             ra305x_serial_init, 
             ra305x_serial_lookup,     // Serial driver may need initializing
             &ra305x_serial_channel2
    );
    
#endif // !!CYGPKG_IO_SERIAL_MIPS_RA305X_SERIAL_1

// Internal function to actually configure the hardware to desired baud rate, etc.
static bool ra305x_serial_config_port(serial_channel *chan, cyg_serial_info_t *new_config, bool init)
{
    ra305x_serial_info_t *ra305x_chan = (ra305x_serial_info_t *)chan->dev_priv;
    cyg_uint32 baud_divisor = select_baud[new_config->baud];
    cyg_int32 parity, wordlen, stopbit;
    cyg_uint32 int_mask;

    if (baud_divisor == 0)
        return false;    // Invalid baud rate selected

    if((parity=select_parity[new_config->parity]) == -1)
        return false;    // Unsupported parity option
    
    if((wordlen=select_word_length[(new_config->word_length -
           CYGNUM_SERIAL_WORD_LENGTH_5)]) == -1 )
        return false;    // Unsupported word length
    
    if((stopbit=select_stop_bits[new_config->stop]) == -1)
        return false;    // Unsupported stop bit

    if (init) {
        /*  Reset UART controller by generating an one to zero transition */
        if (ra305x_chan->base == HAL_RA305X_UART_BASE) {
	    HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) = RA305X_SYSCTL_UART_RST;
        } else if (ra305x_chan->base == HAL_RA305X_UARTLITE_BASE) {
	    HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) = RA305X_SYSCTL_UARTL_RST;
        }
#ifdef CONFIG_MT7628_ASIC
		else if (ra305x_chan->base == HAL_RA305X_UART_LITE3_BASE)
		HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) = RA305X_SYSCTL_UARTL3_RST;
#endif
        HAL_SYSCTL_REG(RA305X_SYSCTL_RESET) = 0;
    }

    // Disable port interrupts while changing hardware
    int_mask = RA305X_UARTREG(ra305x_chan, UART_IER);
    RA305X_UARTREG(ra305x_chan, UART_IER) = 0;


    // Set databits, stopbits and parity.
    RA305X_UARTREG(ra305x_chan, UART_LCR) = parity | wordlen | stopbit;
    
    // Set baud rate.
    baud_divisor = RA_SERIAL_BAUD_DIVISOR(baud_divisor);
    RA305X_UARTREG(ra305x_chan, UART_LCR) |= UART_LCR_DLAB;
    RA305X_UARTREG(ra305x_chan, UART_DLL) = baud_divisor & 0XFF;
    RA305X_UARTREG(ra305x_chan,UART_DLM) =( baud_divisor>>8);
    RA305X_UARTREG(ra305x_chan, UART_LCR) &= ~UART_LCR_DLAB;

    
    if (init) {
    	// Enable and clear FIFOs
	RA305X_UARTREG(ra305x_chan, UART_FCR) = UART_FCR_FIFOEN | UART_FCR_RXRST | UART_FCR_TXRST | 0x80;
    	

#ifndef CYGPKG_IO_SERIAL_MIPS_RA305X_POLLED_MODE    
        if (chan->out_cbuf.len != 0) {
            // Enable rx interrupt
            int_mask = 	UART_IER_ERBFI;
         
        } else
#endif
	{
            int_mask = 0;
        }
    }
    
    // Restore interrupt mask
    RA305X_UARTREG(ra305x_chan, UART_IER) = int_mask;

    if (new_config != &chan->config) {
        chan->config = *new_config;
    }
    return true;
}


// Function to initialize the device.  Called at bootstrap time.
static bool ra305x_serial_init(struct cyg_devtab_entry *tab)
{
    serial_channel *chan = (serial_channel *)tab->priv;
    ra305x_serial_info_t *ra305x_chan = (ra305x_serial_info_t *)chan->dev_priv;
//#ifdef CYGDBG_IO_INIT
    diag_printf("ra305x SERIAL init - dev: %x.%d\n", ra305x_chan->base, ra305x_chan->int_num);
//#endif
    (chan->callbacks->serial_init)(chan);  // Really only required for interrupt driven devices
    if (chan->out_cbuf.len != 0) {
        cyg_drv_interrupt_create(ra305x_chan->int_num,
                                 0,         // can change IRQ0 priority
                                 (cyg_addrword_t)chan,   //  Data item passed to interrupt handler
                                 ra305x_serial_isr,
                                 ra305x_serial_dsr,
                                 &ra305x_chan->serial_interrupt_handle,
                                 &ra305x_chan->serial_interrupt);
        cyg_drv_interrupt_attach(ra305x_chan->serial_interrupt_handle);
        cyg_drv_interrupt_unmask(ra305x_chan->int_num);
    }
    ra305x_serial_config_port(chan, &chan->config, true);
    return true;
}


// This routine is called when the device is "looked" up (i.e. attached)
static Cyg_ErrNo ra305x_serial_lookup(struct cyg_devtab_entry **tab, 
                  struct cyg_devtab_entry *sub_tab,
                  const char *name)
{
    serial_channel *chan = (serial_channel *)(*tab)->priv;
    (chan->callbacks->serial_init)(chan);  // Really only required for interrupt driven devices
    return ENOERR;
}



// Send a character to the device output buffer.
// Return 'true' if character is sent to device
static bool ra305x_serial_putc(serial_channel *chan, unsigned char c)
{
    ra305x_serial_info_t *ra305x_chan = (ra305x_serial_info_t *)chan->dev_priv;

    if(RA305X_UARTREG(ra305x_chan, UART_LSR)& UART_LSR_THRE)
    {
    	RA305X_UARTREG(ra305x_chan, UART_TBR) = c;
    	return true;
    }
    
    return false;
    
}


// Fetch a character from the device input buffer, waiting if necessary
static unsigned char ra305x_serial_getc(serial_channel *chan)
{
    unsigned long c;
    ra305x_serial_info_t *ra305x_chan = (ra305x_serial_info_t *)chan->dev_priv;

    while (!(RA305X_UARTREG(ra305x_chan, UART_LSR) & UART_LSR_DR));
    c = RA305X_UARTREG(ra305x_chan, UART_RBR);
    
    return c&0xff;
}


// Set up the device characteristics; baud rate, etc.
static Cyg_ErrNo ra305x_serial_set_config(serial_channel *chan, cyg_uint32 key,
                        const void *xbuf, cyg_uint32 *len)
{
    switch (key) {
    case CYG_IO_SET_CONFIG_SERIAL_INFO:
      {
        cyg_serial_info_t *config = (cyg_serial_info_t *)xbuf;
        if ( *len < sizeof(cyg_serial_info_t) ) {
            return -EINVAL;
        }
        *len = sizeof(cyg_serial_info_t);
        if ( true != ra305x_serial_config_port(chan, config, false) )
            return -EINVAL;
      }
      break;
    default:
        return -EINVAL;
    }
    return ENOERR;
}


// Enable the transmitter on the device
static void ra305x_serial_start_xmit(serial_channel *chan)
{
    ra305x_serial_info_t *ra305x_chan = (ra305x_serial_info_t *)chan->dev_priv;

    // Enable xmit interrupt
    RA305X_UARTREG(ra305x_chan, UART_IER) |= UART_IER_ETBEI;

    // We should not need to call this here.  THRE Interrupts are enabled, and the DSR
    // below calls this function.  However, sometimes we get called with Master Interrupts
    // disabled, and thus the DSR never runs.  This is unfortunate because it means we
    // will be doing multiple processing steps for the same thing.
    (chan->callbacks->xmt_char)(chan);
}


// Disable the transmitter on the device
static void ra305x_serial_stop_xmit(serial_channel *chan)
{
    ra305x_serial_info_t *ra305x_chan = (ra305x_serial_info_t *)chan->dev_priv;

    RA305X_UARTREG(ra305x_chan, UART_IER) &= ~UART_IER_ETBEI;
}


// Serial I/O - low level interrupt handler (ISR)
static cyg_uint32 ra305x_serial_isr(cyg_vector_t vector, cyg_addrword_t data)
{
    serial_channel *chan = (serial_channel *)data;
    ra305x_serial_info_t *ra305x_chan = (ra305x_serial_info_t *)chan->dev_priv;

    cyg_drv_interrupt_mask(ra305x_chan->int_num);
	//#ifdef CONFIG_MT7628_ASIC
	// diag_printf("IRQ_MASK  %d\n",HAL_INTCL_REG(0x70));
	//#endif
    cyg_drv_interrupt_acknowledge(ra305x_chan->int_num);
    return CYG_ISR_CALL_DSR;  // Cause DSR to be run
}


// Serial I/O - high level interrupt handler (DSR)
static void ra305x_serial_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    serial_channel *chan = (serial_channel *)data;
    ra305x_serial_info_t *ra305x_chan = (ra305x_serial_info_t *)chan->dev_priv;
    cyg_uint32 iir, c;

	//diag_printf("%s\n",__FUNCTION__);
    iir = RA305X_UARTREG(ra305x_chan, UART_IIR) & UART_IIR_ID_MASK;
    while (iir != UART_IIR_NONE) {
        switch(iir) {
            case UART_IIR_RX_AVIL:
            case UART_IIR_RX_TIMEOUT:
		while (RA305X_UARTREG(ra305x_chan, UART_LSR) & UART_LSR_DR) {
                    c = RA305X_UARTREG(ra305x_chan, UART_RBR) & 0xff;
		    (chan->callbacks->rcv_char)(chan, c);
		}
		break;
            case UART_IIR_TX_EMPTY:
		(chan->callbacks->xmt_char)(chan);
		break;
            case UART_IIR_MODEM_STATUS:
            case UART_IIR_RX_LINE_STATUS:
		break;
            default:
		break;
            }
        iir = RA305X_UARTREG(ra305x_chan, UART_IIR) & UART_IIR_ID_MASK;
    }
    
    cyg_drv_interrupt_unmask(ra305x_chan->int_num);
	
//#ifdef CONFIG_MT7628_ASIC
//	 diag_printf("IRQ_MASK 1 %d\n",HAL_INTCL_REG(0x70));
//#endif
}

#endif


//-------------------------------------------------------------------------
// EOF ra305x_serial.c

