#define NFCONF (*((volatile unsigned long *)0x4E000000))
#define NFCONT (*((volatile unsigned long *)0x4E000004))
#define NFCMMD (*((volatile unsigned char *)0x4E000008))
#define NFADDR (*((volatile unsigned char *)0x4E00000C))
#define NFDATA (*((volatile unsigned char *)0x4E000010))
#define NFSTAT (*((volatile unsigned char *)0x4E000020))

static void nand_read_ll(unsigned char *buf, unsigned int addr, \
	unsigned int len);

static int is_boot_from_nor_flash(void)
{
	volatile int *p = (volatile int *)0;
	int val;

	val = *p;
	*p = 0x12345678;
	if (*p == 0x12345678) {
		*p = val;
		return 0; // boot from nand
	} else
		return 1;
}
	
void copy_code_to_sdram(unsigned char *dst, unsigned char *src, \
	unsigned int len)
{
	int i = 0;
	if (is_boot_from_nor_flash()) 
		while (i < len) {
			dst[i] = src[i];
			i++;
		}
	else 
		nand_read_ll(dst, (unsigned int)src, len);
}

void nand_init_ll(void)
{
#define TACLS	0
#define TWRPH0	1
#define TWRPH1	0
	NFCONF	= (TACLS<<12)|(TWRPH0<<8)|(TWRPH1<<4);
	NFCONT	= (1<<4)|(1<<1)|(1<<0);
}

static void nand_select(void)
{
	NFCONT &= ~(1<<1);
}

static void nand_deselect(void)
{
	NFCONT |= (1<<1);
}

static void nand_cmd(unsigned char cmd)
{
	volatile int i;
	NFCMMD = cmd;
	for (i = 0; i < 10; i++);
}

static void nand_addr(unsigned int addr)
{
	unsigned int col = addr % 2048;
	unsigned int page = addr / 2048;
	volatile int i;
	
	NFADDR = col & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR = (col >> 8) & 0xff;
	for (i = 0; i < 10; i++);

	NFADDR = page & 0xff;
	for (i = 0; i < 10; i++);
	NFADDR = (page>>8)& 0xff;
	for (i = 0; i < 10; i++);
	NFADDR = (page>>16)& 0xff;
	for (i = 0; i < 10; i++);
}

static void nand_wait_ready(void)
{
	while (!(NFSTAT & 1));
}

static unsigned char nand_data(void)
{
	return NFDATA;
}

static void nand_read_ll(unsigned char *buf, unsigned int addr, \
	unsigned int len)
{
	int col = addr % 2048;
	int i = 0;
	
	nand_select();
	while (i < len) {
		nand_cmd(0x00);
		nand_addr(addr);
		nand_cmd(0x30);
		nand_wait_ready();
		
		for (; (col < 2048)&&(i<len); col++) {
			buf[i] = nand_data();
			i++;
			addr++;
		}
		col = 0;
	}
	nand_deselect();
}
