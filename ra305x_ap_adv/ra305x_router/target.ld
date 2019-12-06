OUTPUT_FORMAT("elf32-bigmips", "elf32-bigmips",
              "elf32-littlemips")


OUTPUT_ARCH(mips:isa32)

STARTUP(vectors.o)
ENTRY(reset_vector)

INPUT(extras.o)
INPUT(drivers.o)
INPUT(apps.o)

GROUP(libtarget.a libgcc.a libsupc++.a)
MEMORY
{
    ram : ORIGIN = 0x80000000, LENGTH = 8*0x100000
}

SECTIONS
{
    .jump  0x80000400 : { KEEP (*(.jump_to_start)) } > ram
    .rom_vectors ALIGN (0x4) : { KEEP (*(.utlb_vector)) . = ALIGN(0x80); KEEP(*(.other_vector)) KEEP(*(.debug_vector)) KEEP (*(.reset_vector)) } > ram
    .interp ALIGN (0x4) : { *(.interp) } > ram .hash : AT ((LOADADDR (.interp) + SIZEOF (.interp) + 0x40 - 1) & ~ (0x40 - 1)) { *(.hash) } > ram .dynsym : AT ((LOADADDR (.hash) + SIZEOF (.hash) + 0x40 - 1) & ~ (0x40 - 1)) { *(.dynsym) } > ram .dynstr : AT ((LOADADDR (.dynsym) + SIZEOF (.dynsym) + 0x40 - 1) & ~ (0x40 - 1)) { *(.dynstr) } > ram .gnu.version : AT ((LOADADDR (.dynstr) + SIZEOF (.dynstr) + 0x40 - 1) & ~ (0x40 - 1)) { *(.gnu.version) } > ram .gnu.version_d : AT ((LOADADDR (.gnu.version) + SIZEOF (.gnu.version) + 0x40 - 1) & ~ (0x40 - 1)) { *(.gnu.version_d) } > ram .gnu.version_r : AT ((LOADADDR (.gnu.version_d) + SIZEOF (.gnu.version_d) + 0x40 - 1) & ~ (0x40 - 1)) { *(.gnu.version_r) } > ram .plt : AT ((LOADADDR (.gnu.version_r) + SIZEOF (.gnu.version_r) + 0x40 - 1) & ~ (0x40 - 1)) { *(.plt) } > ram
    .rel.text : { *(.rel.text) *(.rel.text.*) *(.rel.gnu.linkonce.t*) } > ram .rela.text : { *(.rela.text) *(.rela.text.*) *(.rela.gnu.linkonce.t*) } > ram .rel.data : { *(.rel.data) *(.rel.data.*) *(.rel.gnu.linkonce.d*) } > ram .rela.data : { *(.rela.data) *(.rela.data.*) *(.rela.gnu.linkonce.d*) } > ram .rel.rodata : { *(.rel.rodata) *(.rel.rodata.*) *(.rel.gnu.linkonce.r*) } > ram .rela.rodata : { *(.rela.rodata) *(.rela.rodata.*) *(.rela.gnu.linkonce.r*) } > ram .rel.got : { *(.rel.got) } > ram .rela.got : { *(.rela.got) } > ram .rel.ctors : { *(.rel.ctors) } > ram .rela.ctors : { *(.rela.ctors) } > ram .rel.dtors : { *(.rel.dtors) } > ram .rela.dtors : { *(.rela.dtors) } > ram .rel.init : { *(.rel.init) } > ram .rela.init : { *(.rela.init) } > ram .rel.fini : { *(.rel.fini) } > ram .rela.fini : { *(.rela.fini) } > ram .rel.bss : { *(.rel.bss) } > ram .rela.bss : { *(.rela.bss) } > ram .rel.plt : { *(.rel.plt) } > ram .rela.plt : { *(.rela.plt) } > ram .rel.dyn : { *(.rel.dyn) } > ram
    .init ALIGN (0x4) : { . = .; KEEP (*(.init)) } > ram =0
    .text ALIGN (0x4) : { _stext = .; _ftext = . ; *(.text) *(.text.*) *(.stub) *(.gnu.warning) *(.gnu.linkonce.t*) *(.mips16.fn.*) *(.mips16.call.*) } > ram =0 _etext = .; PROVIDE (etext = .);
    .fini ALIGN (0x4) : { . = .; KEEP (*(.fini)) } > ram =0
    .rodata ALIGN (0x8) : { . = .; *(.rodata) *(.rodata.*) *(.gnu.linkonce.r*) } > ram
    .rodata1 ALIGN (0x8) : { . = .; *(.rodata1) *(.rodata1.*) } > ram
    .data ALIGN (0x8) : { __ram_data_start = ABSOLUTE (.); _fdata = . ; *(.data) *(.data.*) *(.gnu.linkonce.d*) *( .2ram.*) . = ALIGN (8); SORT(CONSTRUCTORS) } > ram __rom_data_start = LOADADDR(.data);
    .data1 ALIGN (0x8) : { . = .; *(.data1) *(.data1.*) } > ram
    .eh_frame ALIGN (0x4) : { . = .; *(.eh_frame) } > ram
    .gcc_except_table ALIGN (0x4) : { . = .; *(.gcc_except_table) } > ram
    .ctors ALIGN (0x4) : { . = .; KEEP (*crtbegin.o(.ctors)) __CTOR_LIST__ = .; PROVIDE (__CTOR_LIST__ = .); KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors)) KEEP (*(SORT(.ctors.*))) KEEP (*(.ctors)) __CTOR_END__ = .; PROVIDE (__CTOR_END__ = .); } > ram
    .dtors ALIGN (0x4) : { . = .; KEEP (*crtbegin.o(.dtors)) __DTOR_LIST__ = .; PROVIDE (__DTOR_LIST__ = .); KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors)) KEEP (*(SORT(.dtors.*))) KEEP (*(.dtors)) __DTOR_END__ = .; PROVIDE (__DTOR_END__ = .); } > ram
    .devtab ALIGN (0x4) : { . = .; KEEP(*( SORT (.ecos.table.*))) ; } > ram
    _gp = ALIGN(16) + 0x7ff0; .got ALIGN (0x4) : { . = .; *(.got.plt) *(.got) } > ram
    .dynamic ALIGN (0x4) : { . = .; *(.dynamic) } > ram
    .sdata ALIGN (0x4) : { . = .; *(.sdata) *(.sdata.*) *(.gnu.linkonce.s*) } > ram
    .lit8 ALIGN (0x8) : { . = .; *(.lit8) } > ram
    .lit4 : AT ((LOADADDR (.lit8) + SIZEOF (.lit8) + 0x40 - 1) & ~ (0x40 - 1)) { . = .; *(.lit4) } > ram __ram_data_end = .; _edata = . ; PROVIDE (edata = .);
    __bss_start = .; _fbss = .; .sbss ALIGN (0x8) : { . = .; *(.dynsbss) *(.sbss) *(.sbss.*) *(.scommon) } > ram
    .bss ALIGN (0x8) : { *(.dynbss) *(.bss) *(.bss.*) *(COMMON) } > ram __bss_end = .;
    __heap1 = ALIGN (0x8);
	__heap1_end = 0x80000000 + 8*0x100000 - 16384*2 - 200*1024 - 2400*1024 - 0*1024;
    __pci_window = 0x81f00000; . = __pci_window + 0x100000;
    . = ALIGN(4); _end = .; PROVIDE (end = .); .stab 0 : { *(.stab) } .stabstr 0 : { *(.stabstr) } .stab.excl 0 : { *(.stab.excl) } .stab.exclstr 0 : { *(.stab.exclstr) } .stab.index 0 : { *(.stab.index) } .stab.indexstr 0 : { *(.stab.indexstr) } .comment 0 : { *(.comment) } .debug 0 : { *(.debug) } .line 0 : { *(.line) } .debug_srcinfo 0 : { *(.debug_srcinfo) } .debug_sfnames 0 : { *(.debug_sfnames) } .debug_aranges 0 : { *(.debug_aranges) } .debug_pubnames 0 : { *(.debug_pubnames) } .debug_info 0 : { *(.debug_info) } .debug_abbrev 0 : { *(.debug_abbrev) } .debug_line 0 : { *(.debug_line) } .debug_frame 0 : { *(.debug_frame) } .debug_str 0 : { *(.debug_str) } .debug_loc 0 : { *(.debug_loc) } .debug_macinfo 0 : { *(.debug_macinfo) } .debug_weaknames 0 : { *(.debug_weaknames) } .debug_funcnames 0 : { *(.debug_funcnames) } .debug_typenames 0 : { *(.debug_typenames) } .debug_varnames 0 : { *(.debug_varnames) } .gptab.sdata : { *(.gptab.data) *(.gptab.sdata) } .gptab.sbss : { *(.gptab.bss) *(.gptab.sbss) } /DISCARD/ 0 : { *(.mdebug) }
}
log_buf_area = __heap1_end;
net_mbufs_area = log_buf_area + 16384*2;
net_clusters_area = net_mbufs_area + 200*1024;
net_clusters_area_4k = net_clusters_area + 2400*1024;
hal_vsr_table = 0x80000200;
hal_virtual_vector_table = 0x80000300;

