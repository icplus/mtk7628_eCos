
OUTPUT_FORMAT("elf32-littlemips")
OUTPUT_ARCH(mips)

/***************************************************************************
 *                             SECTION LAYOUT
 **************************************************************************/
SECTIONS
{
    .jump : { KEEP (*(.jump_to_start)) }

    .text ALIGN (0x4) : { _ftext = . ; *(.text) *(.text.*) }
	_etext = .;
    .rodata ALIGN (0x8) : { . = .; *(.rodata) *(.rodata.*) }
    .data ALIGN (0x8) : { _fdata = . ; *.o(.data) *.o(.data.*) }
    _gp = ALIGN(0x100) ;
    .sdata ALIGN (0x4) : { . = .; *(.sdata) *(.sdata.*) }

	. = ALIGN(16) ;

    zimage : 
    { 
        bin.gz*(.data)
    }
	_zimage_sz = SIZEOF(zimage);

	. = ALIGN(16) ;

	webdata : 
    { 
		_webdata_start = . ;
        webdata.bin*(.data) ;
		_webdata_end = . ;
    }
	_webdata_sz = SIZEOF(webdata);

	. = ALIGN(16) ;

	zprofile :
	{
		profile.txt.*(.data)
	}
	_zprofile_sz = SIZEOF(zprofile);
  
	. = ALIGN(4);
    __bss_start = .; _fbss = .;
	.sbss ALIGN (0x8) : { . = .; *(.sbss) *(.sbss.*) }
    .bss ALIGN (0x8) : { *(.bss) *(.bss.*) *(COMMON) }
	__bss_end = .;

	_stack_bot = . ;
	. += 4096;
	_stack_top = . ;

    . = ALIGN(4); _end = .; PROVIDE (end = .);
	__heap1 = ALIGN(0x10) ;
}
