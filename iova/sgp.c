/*
 * SGP.C: PC-88VA Super Graphic Processor
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"memoryva.h"
#include	"gvramva.h"
#include	"sgp.h"
#include	"bmsio.h"

#if defined(SUPPORT_PC88VA)

#define		SWAPWORD(x)	( ((x) << 8) | ((x) >> 8) )

enum {
	// func
	FUNC_FETCH_COMMAND		= 0,
	FUNC_EXEC_BITBLT,
	FUNC_EXEC_BITBLT_HD,
	FUNC_EXEC_CLS,
	FUNC_EXEC_LINE_X,
	FUNC_EXEC_LINE_Y,
};

		_SGP	sgp;


// ---- 

static REG16 MEMCALL mainw_rd(UINT32 address) {
	return *(REG16 *)(mem + address);
}

static void MEMCALL mainw_wt(UINT32 address, REG16 value) {
	*(REG16 *)(mem + address) = value;
}

static REG16 MEMCALL bmsw_rd(UINT32 address) {
	address -= 0x080000L;
	if (!bmsio.nomem) {
		return *(REG16 *)(bmsiowork.bmsmem + (bmsio.bank << 17) + address);
	}
	else {
		return 0xffff;
	}
}

static void MEMCALL bmsw_wt(UINT32 address, REG16 value) {
	address -= 0x080000L;
	if (!bmsio.nomem) {
		*(REG16 *)(bmsiowork.bmsmem + (bmsio.bank << 17) + address) = value;
	}
}

static REG16 MEMCALL tvramw_rd(UINT32 address) {
	address -= 0x180000L;
	return *(REG16 *)(textmem + address);
}

static void MEMCALL tvramw_wt(UINT32 address, REG16 value) {
	address -= 0x180000L;
	*(UINT16 *)(textmem + address) = value;
}

static REG16 MEMCALL gvramw_rd(UINT32 address) {
	address -= 0x200000L;
	return *(REG16 *)(grphmem + address);
}

static void MEMCALL gvramw_wt(UINT32 address, REG16 value) {
	address -= 0x200000L;
	*(UINT16 *)(grphmem + address) = value;
}

static REG16 MEMCALL knj1w_rd(UINT32 address) {
	// TODO
	return 0;
}

static REG16 MEMCALL knj2w_rd(UINT32 address) {
	// TODO
	return 0;
}

static void MEMCALL knj2w_wt(UINT32 address, REG16 value) {
	// TODO
}

static REG16 MEMCALL nonw_rd(UINT32 address) {
	return 0xffff;
}

static void MEMCALL nonw_wt(UINT32 address, REG16 value) {
}


typedef void (MEMCALL * MEM16WRITE)(UINT32 address, REG16 value);
typedef REG16 (MEMCALL * MEM16READ)(UINT32 address);

static MEM16READ rd16[0x40] = {
	mainw_rd,	// 00xxxx
	mainw_rd,	// 01xxxx
	mainw_rd,	// 02xxxx
	mainw_rd,	// 03xxxx
	mainw_rd,	// 04xxxx
	mainw_rd,	// 05xxxx
	mainw_rd,	// 06xxxx
	mainw_rd,	// 07xxxx
	bmsw_rd,	// 08xxxx
	bmsw_rd,	// 09xxxx
	nonw_rd,	// 0axxxx	ToDo
	nonw_rd,	// 0bxxxx	ToDo
	nonw_rd,	// 0cxxxx	ToDo
	nonw_rd,	// 0dxxxx	ToDo
	nonw_rd,	// 0exxxx	ToDo
	nonw_rd,	// 0fxxxx	ToDo

	knj1w_rd,	// 10xxxx
	knj1w_rd,	// 11xxxx
	knj1w_rd,	// 12xxxx
	knj1w_rd,	// 13xxxx
	knj2w_rd,	// 14xxxx
	knj2w_rd,	// 15xxxx
	knj2w_rd,	// 16xxxx
	knj2w_rd,	// 17xxxx
	tvramw_rd,	// 18xxxx
	tvramw_rd,	// 19xxxx	
	tvramw_rd,	// 1axxxx	
	tvramw_rd,	// 1bxxxx	
	nonw_rd,	// 1cxxxx
	nonw_rd,	// 1dxxxx
	nonw_rd,	// 1exxxx
	nonw_rd,	// 1fxxxx

	gvramw_rd,	// 20xxxx
	gvramw_rd,	// 21xxxx
	gvramw_rd,	// 22xxxx
	gvramw_rd,	// 23xxxx
	nonw_rd,	// 24xxxx
	nonw_rd,	// 25xxxx
	nonw_rd,	// 26xxxx
	nonw_rd,	// 27xxxx
	nonw_rd,	// 28xxxx
	nonw_rd,	// 29xxxx
	nonw_rd,	// 2axxxx
	nonw_rd,	// 2bxxxx
	nonw_rd,	// 2cxxxx
	nonw_rd,	// 2dxxxx
	nonw_rd,	// 2exxxx
	nonw_rd,	// 2fxxxx

	nonw_rd,	// 30xxxx
	nonw_rd,	// 31xxxx
	nonw_rd,	// 32xxxx
	nonw_rd,	// 33xxxx
	nonw_rd,	// 34xxxx
	nonw_rd,	// 35xxxx
	nonw_rd,	// 36xxxx
	nonw_rd,	// 37xxxx
	nonw_rd,	// 38xxxx
	nonw_rd,	// 39xxxx
	nonw_rd,	// 3axxxx
	nonw_rd,	// 3bxxxx
	nonw_rd,	// 3cxxxx
	nonw_rd,	// 3dxxxx
	nonw_rd,	// 3exxxx
	nonw_rd,	// 3fxxxx

};


static MEM16WRITE wt16[0x40] = {
	mainw_wt,	// 00xxxx
	mainw_wt,	// 01xxxx
	mainw_wt,	// 02xxxx
	mainw_wt,	// 03xxxx
	mainw_wt,	// 04xxxx
	mainw_wt,	// 05xxxx
	mainw_wt,	// 06xxxx
	mainw_wt,	// 07xxxx
	bmsw_wt,	// 08xxxx
	bmsw_wt,	// 09xxxx
	nonw_wt,	// 0axxxx	ToDo
	nonw_wt,	// 0bxxxx	ToDo
	nonw_wt,	// 0cxxxx	ToDo
	nonw_wt,	// 0dxxxx	ToDo
	nonw_wt,	// 0exxxx	ToDo
	nonw_wt,	// 0fxxxx	ToDo

	nonw_wt,	// 10xxxx
	nonw_wt,	// 11xxxx
	nonw_wt,	// 12xxxx
	nonw_wt,	// 13xxxx
	knj2w_wt,	// 14xxxx
	knj2w_wt,	// 15xxxx
	knj2w_wt,	// 16xxxx
	knj2w_wt,	// 17xxxx
	tvramw_wt,	// 18xxxx
	tvramw_wt,	// 19xxxx	
	tvramw_wt,	// 1axxxx	
	tvramw_wt,	// 1bxxxx	
	nonw_wt,	// 1cxxxx
	nonw_wt,	// 1dxxxx
	nonw_wt,	// 1exxxx
	nonw_wt,	// 1fxxxx

	gvramw_wt,	// 20xxxx
	gvramw_wt,	// 21xxxx
	gvramw_wt,	// 22xxxx
	gvramw_wt,	// 23xxxx
	nonw_wt,	// 24xxxx
	nonw_wt,	// 25xxxx
	nonw_wt,	// 26xxxx
	nonw_wt,	// 27xxxx
	nonw_wt,	// 28xxxx
	nonw_wt,	// 29xxxx
	nonw_wt,	// 2axxxx
	nonw_wt,	// 2bxxxx
	nonw_wt,	// 2cxxxx
	nonw_wt,	// 2dxxxx
	nonw_wt,	// 2exxxx
	nonw_wt,	// 2fxxxx

	nonw_wt,	// 30xxxx
	nonw_wt,	// 31xxxx
	nonw_wt,	// 32xxxx
	nonw_wt,	// 33xxxx
	nonw_wt,	// 34xxxx
	nonw_wt,	// 35xxxx
	nonw_wt,	// 36xxxx
	nonw_wt,	// 37xxxx
	nonw_wt,	// 38xxxx
	nonw_wt,	// 39xxxx
	nonw_wt,	// 3axxxx
	nonw_wt,	// 3bxxxx
	nonw_wt,	// 3cxxxx
	nonw_wt,	// 3dxxxx
	nonw_wt,	// 3exxxx
	nonw_wt,	// 3fxxxx

};

/*
static BYTE sgp_memoryread(UINT32 address) {
//...
}
*/

/*
	����:
		address		�A�h���X(����)
*/
static REG16 sgp_memoryread_w(UINT32 address) {
	CPU_REMCLOCK -= 4;		// ToDo:�{���́ACPU���������A�N�Z�X���悤�Ƃ��Ă����ꍇ��
							//      �̂ݑ҂���������B���̎����ł͏�ɑ҂�������B
							//      CPU��rep movsw�Ȃǒ����Ԃ�������ꍇ�A�����
							//		sgp_step�Ń������]������ʂɐ����ACPU_REMCLOCK���傫��
							//      ���ۂ��A�C�x���g�̔������x���\��������B
							//      4�Ƃ����l�ɂ͍����Ȃ��B
	return rd16[(address >> 16) & 0x3f](address & 0x3fffffL);
}

/*
	����:
		address		�A�h���X(����)
*/
static void sgp_memorywrite_w(UINT32 address, REG16 value) {
	CPU_REMCLOCK -= 4;		// ToDo: sgp_remoryread_w���Q��
	wt16[(address >> 16) & 0x3f](address & 0x3fffffL, value);
}

// ---- 
static int dotcountmax[4] = {0x10, 0x04, 0x02, 0x01};
static int bpp[4] = {1, 4, 8, 16};

static void fetch_command(void);

static void fetch_block(UINT32 address, SGP_BLOCK block) {
	UINT16 dat;

	dat = sgp_memoryread_w(address);
	block->scrnmode = dat & 0x03;
	block->dot = (dat >> 4) & (dotcountmax[block->scrnmode] -1);
	block->width = sgp_memoryread_w(address + 2) & 0x3fff;	/* VA2��14bit�܂ŉ\ */
	block->height = sgp_memoryread_w(address + 4);			/* VA2��16bit�܂ŉ\ */
	block->fbw = sgp_memoryread_w(address + 6) & 0xfffe;	/* bit1���L�� */
	block->address = sgp_memoryread_w(address + 8) & 0xfffe | ((UINT32)sgp_memoryread_w(address + 10) << 16);

	// ToDo �Ƃ肠�����̂��܂��킹(R-TYPE)
	if (block->fbw < 0) block->fbw += 2;
}

static void init_block(SGP_BLOCK block) {
	block->lineaddress = block->address;
	block->nextaddress = block->address;
	block->ycount = block->height;
}

static void read_word(SGP_BLOCK block) {
	UINT16 dat;

	dat = sgp_memoryread_w(block->nextaddress);
	block->buf = SWAPWORD(dat);
	//block->nextaddress += 2;
	block->dotcount = dotcountmax[block->scrnmode];
}

/*
static void write_word(SGP_BLOCK block) {
	UINT16 dat;
	UINT16 mask;

	dat = (block->buf << 8) | (block->buf >> 8);
	mask = (block->mask << 8) | (block->mask >> 8);
	dat = dat & mask | (sgp_memoryread_w(block->nextaddress) & ~mask);
	sgp_memorywrite_w(block->nextaddress, dat);
	block->nextaddress += 2;
	block->dotcount = dotcountmax[block->scrnmode];
}
*/

static UINT16 logicalop(UINT16 dat, UINT16 dest, UINT16 *_mask) {
	UINT16 mask;

	mask = *_mask;

	switch (sgp.bltmode & SGP_BLTMODE_OP) {
	case 0:		// 0
		dat = 0;
		break;
	case 1:		// S AND D
		dat = dat & dest;
		break;
	case 2:		// NOT(S) AND D
		dat = ~dat & dest;
		break;
	case 3:		// NOP
		mask = 0;
		break;
	case 4:		// S AND NOT(D)
		dat = dat & ~dest;
		break;
	case 5:		// S
	default:
		break;
	case 6:		// S XOR D
		dat = dat ^ dest;
		break;
	case 7:		// S OR D
		dat = dat | dest;
		break;
	case 8:		// NOT(S OR D)
		dat = ~(dat | dest);
		break;
	case 9:		// NOT(S XOR D)
		dat = ~(dat ^ dest);
		break;
	case 0x0a:	// NOT(S)
		dat = ~dat;
		break;
	case 0x0b:	// NOT(S) OR D
		dat = ~dat | dest;
		break;
	case 0x0c:	// NOT(D)
		dat = ~dest;
		break;
	case 0x0d:	// S OR NOT(D)
		dat = dat | ~dest;
		break;
	case 0x0e:	// NOT(S AND D)
		dat = ~(dat & dest);
		break;
	case 0x0f:	// 1
		dat = 0xffff;
		break;
	}

	*_mask = mask;
	return dat;
}


/*
dat ���s�N�Z���f�[�^��Ƃ��āA�s�N�Z���l��0�̕�����0�A��0�̕�����1�ɂ����}�X�N��
�Ԃ�
*/
static UINT16 zeromask(UINT16 dat, int scrnmode) {
	UINT16 mask = 0;
	int BPP = bpp[scrnmode];
	UINT16 pixmask  = ~(0xffff >> BPP); // 4bpp�Ȃ�0xf000
	UINT16 maskelem = ~(0xffff << BPP); // 4bpp�Ȃ�0x000f
	int i;

	for (i = 0; i < dotcountmax[scrnmode]; i++) {
		mask <<= BPP;
		if (dat & pixmask) mask |= maskelem;
		dat <<= BPP;
	}

	return mask;
}


static void write_dest(void) {
	UINT16 dat;
	UINT16 mask;
	UINT16 dest;

	dat = SWAPWORD(sgp.newval);
	mask = SWAPWORD(sgp.newvalmask);
	dest = sgp_memoryread_w(sgp.dest.nextaddress);

	dat = logicalop(dat, dest, &mask);

	dat = dat & mask | (dest & ~mask);

	sgp_memorywrite_w(sgp.dest.nextaddress, dat);
#if 0
	sgp.dest.nextaddress += 2;
	sgp.dest.dotcount = dotcountmax[sgp.dest.scrnmode];

#else
	/*
	sgp.dest.nextaddress += 2;
	read_word(&sgp.dest);
	*/
#endif
}


static void write_dest2(void) {
	UINT16 dat;
	UINT16 dest;
	UINT16 datmask;

	dest = sgp_memoryread_w(sgp.dest.nextaddress);
	dest = SWAPWORD(dest);
	datmask = sgp.newvalmask;
	switch (sgp.bltmode & SGP_BLTMODE_TP) {
	case 0x0200:		// �f�X�e�B�l�[�V�����u���b�N��0�̕��������]������
	case 0x0300:		// �֎~ �� 0x0200�Ɠ���
		datmask &= ~zeromask(dest, sgp.dest.scrnmode);
		break;
	}
	dat = logicalop(sgp.newval, dest, &datmask);

	dat = (dest & ~datmask) | (dat & datmask);
	dat = SWAPWORD(dat);
	sgp_memorywrite_w(sgp.dest.nextaddress, dat);
}


static void init_src_line(void) {
	int dot;
	if (sgp.bltmode & SGP_BLTMODE_SF) {
		dot = sgp.dest.dot;
		if (sgp.dest.dot < sgp.src.dot) {
			sgp.src.nextaddress += 2;
		}
	}
	else {
		dot = sgp.src.dot;
	}
	read_word(&sgp.src);
	sgp.src.nextaddress += 2;
	sgp.src.dotcount -= dot;
	sgp.src.buf <<= (dotcountmax[sgp.src.scrnmode] - sgp.src.dotcount) * bpp[sgp.src.scrnmode];

	sgp.src.xcount = sgp.src.width;
}

static void init_dest_line(void) {
	sgp.newval = 0;
	sgp.newvalmask = 0;
#if 0
	//read_word(block);
	sgp.dest.dotcount = dotcountmax[sgp.dest.scrnmode];
	//if (!sf) {
		sgp.dest.dotcount -= sgp.dest.dot;
	//}
	//block->buf <<= (dotcountmax[block->scrnmode] - block->dotcount) * bpp[block->scrnmode];
#else
/* @@1
	read_word(&sgp.dest);
	//sgp.dest.nextaddress-=2;
	//sgp.dest.dotcount = dotcountmax[sgp.dest.scrnmode];
	//if (!sf) {
		sgp.dest.dotcount -= sgp.dest.dot;
	//}
	sgp.dest.buf <<= (dotcountmax[sgp.dest.scrnmode] - sgp.dest.dotcount) * bpp[sgp.dest.scrnmode];
*/
	sgp.dest.dotcount = dotcountmax[sgp.dest.scrnmode]; //@@1
	sgp.dest.dotcount -= sgp.dest.dot;					//@@1
#endif
	sgp.dest.xcount = sgp.dest.width;
}

// HD = 1 �̏ꍇ
static void init_src_line_hd(void) {
	int dot;
	if (sgp.bltmode & SGP_BLTMODE_SF) {
		dot = sgp.dest.dot;
		if (sgp.dest.dot > sgp.src.dot) {
			sgp.src.nextaddress -= 2;
		}
	}
	else {
		dot = sgp.src.dot;
	}

	read_word(&sgp.src);
	sgp.src.nextaddress -= 2;
	sgp.src.dotcount = dot + 1;
	sgp.src.buf >>= (dotcountmax[sgp.src.scrnmode] - sgp.src.dotcount) * bpp[sgp.src.scrnmode];

	sgp.src.xcount = sgp.src.width;
}

// HD = 1 �̏ꍇ
static void init_dest_line_hd(void) {
	sgp.newval = 0;
	sgp.newvalmask = 0;

	read_word(&sgp.dest);
	sgp.dest.dotcount = sgp.dest.dot + 1;
	sgp.dest.buf >>= (dotcountmax[sgp.dest.scrnmode] - sgp.dest.dotcount) * bpp[sgp.dest.scrnmode];
	sgp.dest.xcount = sgp.dest.width;
}


static void setintreq(void) {
	if (sgp.ctrl & SGP_INTF) {
		if (!sgp.intreq) {
			sgp.intreq = 1;
			pic_setirq(8);
			nevent_forceexit();
		}
	}
}




static void cmd_unknown(void) {
	// �����Ȗ���
	TRACEOUT(("SGP: cmd: unknown"));
}

static void cmd_nop(void) {
	// �Ȃɂ����Ȃ�
	TRACEOUT(("SGP: cmd: nop"));
	sgp.remainclock -= 5 * 2;
}

static void cmd_end(void) {
	TRACEOUT(("SGP: cmd: end"));

	sgp.busy &= ~SGP_BUSY;
	setintreq();
}

static void cmd_set_work(void) {
	TRACEOUT(("SGP: cmd: set work"));
	// ��Ɨ̈�(58�o�C�g)�̐ݒ�
	sgp.workmem = sgp_memoryread_w(sgp.pc) & 0xfffe | ((UINT32)sgp_memoryread_w(sgp.pc + 2) << 16);
	sgp.pc += 4;
	sgp.remainclock -= 23 * 2;

}

static void cmd_set_source(void) {
	fetch_block(sgp.pc, &sgp.src);
	TRACEOUT(("SGP: cmd: set source     : dot=%d, mode=%d, w=%d, h=%d, fbw=%d, addr=%08lx", sgp.src.dot, sgp.src.scrnmode, sgp.src.width, sgp.src.height, sgp.src.fbw, sgp.src.address));
	sgp.pc += 12;
	sgp.remainclock -= 106 * 2;
}

static void cmd_set_destination(void) {
	fetch_block(sgp.pc, &sgp.dest);
	TRACEOUT(("SGP: cmd: set destination: dot=%d, mode=%d, w=%d, h=%d, fbw=%d, addr=%08lx", sgp.dest.dot, sgp.dest.scrnmode, sgp.dest.width, sgp.dest.height, sgp.dest.fbw, sgp.dest.address));
	sgp.pc += 12;
	sgp.remainclock -= 106 * 2;
}

static void cmd_set_color(void) {
	sgp.color = sgp_memoryread_w(sgp.pc);
	TRACEOUT(("SGP: cmd: set color: color=0x%x", sgp.color));
	sgp.pc += 2;
	sgp.remainclock -= 10 * 2;
}


static void cmd_bitblt(void) {

	sgp.bltmode = sgp_memoryread_w(sgp.pc);
	sgp.pc += 2;
	sgp.remainclock -= 338 * 2;

	TRACEOUT(("SGP: cmd: bitblt: %04x", sgp.bltmode));

	// BITBLT�̏ꍇ�ASET DESTINATION�Őݒ肵�����A�����͖�������A
	// SET SOURCE�Ŏw�肵�����A���������]�������
	sgp.dest.width = sgp.src.width;
	sgp.dest.height = sgp.src.height;

	init_block(&sgp.src);
	init_block(&sgp.dest);
	if (sgp.bltmode & SGP_BLTMODE_HD) {
		init_src_line_hd();
		init_dest_line_hd();
		sgp.func = FUNC_EXEC_BITBLT_HD;
	}
	else {
		init_src_line();
		init_dest_line();
		sgp.func = FUNC_EXEC_BITBLT;
	}
}

static void cmd_patblt(void) {

	sgp.bltmode = sgp_memoryread_w(sgp.pc);
	sgp.pc += 2;
	sgp.remainclock -= 338 * 2;

	TRACEOUT(("SGP: cmd: patblt: %04x", sgp.bltmode));

	init_block(&sgp.src);
	init_block(&sgp.dest);
	if (sgp.bltmode & SGP_BLTMODE_HD) {
		init_src_line_hd();
		init_dest_line_hd();
		sgp.func = FUNC_EXEC_BITBLT_HD;
	}
	else {
		init_src_line();
		init_dest_line();
		sgp.func = FUNC_EXEC_BITBLT;		// BITBLT�Ƌ��ʂ̃��[�`��
	}
}


static void cmd_line(void) {

	sgp.bltmode = sgp_memoryread_w(sgp.pc);
	sgp.pc += 2;
	fetch_block(sgp.pc, &sgp.dest);

	TRACEOUT(("SGP: cmd: line: %04x, dot=%d, mode=%d, w=%d, h=%d, fbw=%d, addr=%08lx", sgp.bltmode, sgp.dest.dot, sgp.dest.scrnmode, sgp.dest.width, sgp.dest.height, sgp.dest.fbw, sgp.dest.address));
	
	sgp.pc += 12;
	sgp.remainclock -= 109;

	/*if (sgp.dest.width == 0 && sgp.dest.height == 0) {
		// ���@�̏ꍇ�A�������j����N�������Ƃ���A
		// width=height=0x10000 �Ƃ��ē��삵�Ă���Ƒz�������B
	}
	else*/ if (sgp.dest.width < sgp.dest.height) {
		// �c��
		sgp.func = FUNC_EXEC_LINE_Y;
		sgp.dest.ycount = sgp.dest.height;
		sgp.lineslopedenominator = sgp.dest.height - 1;
		sgp.lineslopenumerator   = 
			(sgp.dest.width == 0) ? 0 : sgp.dest.width - 1;
		sgp.lineslopecount = sgp.lineslopedenominator / 2;
		sgp.dest.dotcount = sgp.dest.dot;
		sgp.dest.nextaddress = sgp.dest.address;
	}
	else {
		// �����`�܂��͉���
//		UINT16 dest;

		sgp.func = FUNC_EXEC_LINE_X;
		sgp.dest.xcount = sgp.dest.width;
		sgp.lineslopedenominator = sgp.dest.width - 1;
		sgp.lineslopenumerator   = 
			(sgp.dest.height == 0) ? 0 : sgp.dest.height - 1;
		sgp.lineslopecount = 
			(sgp.lineslopedenominator == 0) ? 0 : (sgp.lineslopedenominator - 1) / 2;
				// -1 ���閾�m�ȍ����͂Ȃ����A�������Ȃ��ƁA
				// ��7����6�̏ꍇ�̕`��_�̈ʒu�����@�ƈ�v���Ȃ��B
		if (sgp.bltmode & SGP_BLTMODE_LINE_HD) {
			// ������
			sgp.dest.dotcount = sgp.dest.dot + 1;
		}
		else {
			// ������
			sgp.dest.dotcount = dotcountmax[sgp.dest.scrnmode] - sgp.dest.dot;
		}
		sgp.dest.nextaddress = sgp.dest.address;
		
		sgp.newval = 0;
		sgp.newvalmask = 0;
//		dest = sgp_memoryread_w(sgp.dest.nextaddress);
//		sgp.dest.buf = SWAPWORD(dest);
//		sgp.dest.buf <<= sgp.dest.dot * bpp[sgp.dest.scrnmode];
	}
}

static void cmd_cls(void) {
	sgp.clsaddr = (UINT32)sgp_memoryread_w(sgp.pc) |
		          (((UINT32)sgp_memoryread_w(sgp.pc + 2)) << 16) &
				  0xfffffffeL;
	sgp.clscount = (UINT32)sgp_memoryread_w(sgp.pc + 4) |
		           (((UINT32)sgp_memoryread_w(sgp.pc + 6)) << 16) ;
	sgp.pc += 8;
	sgp.remainclock -= 26 * 2;

	sgp.func = FUNC_EXEC_CLS;

	TRACEOUT(("SGP: cmd: cls: addr=%0lx, size(word)=%0lx",sgp.clsaddr, sgp.clscount));
}

static void cmd_scan_right(void) {
	TRACEOUT(("SGP: cmd: scan right (not implemented)"));
	//ToDo
}

static void cmd_scan_left(void) {
	TRACEOUT(("SGP: cmd: scan left (not implemented)"));
	//ToDo
}


// ----

static void exec_bitblt(void) {
	UINT16 dat;
//@@1	UINT16 dest;
	UINT16 datmask;
	int BPP = bpp[sgp.dest.scrnmode];
	UINT16 PIXMASK = ~(0xffff << BPP);
	BOOL EXTPIX = sgp.src.scrnmode == 0 && sgp.dest.scrnmode != 0;	// 1bpp����̊g���]��

	if (sgp.src.dotcount == 0) {
		read_word(&sgp.src);
		sgp.src.nextaddress += 2;
	}
	if (EXTPIX) {
		dat = (sgp.src.buf & 0x8000) ? 0xffff : 0;
	}
	else {
		dat = sgp.src.buf >> (16 - BPP);
	}
//@@1	dest = sgp.dest.buf >> (16 - BPP);

	switch (sgp.bltmode & SGP_BLTMODE_TP) {
	case 0x0000:		// �\�[�X�����̂܂ܓ]��
	default:	//@@1
		datmask = 0xffff;
		break;
	case 0x0100:		// �\�[�X��0�̕����͓]�����Ȃ�
		datmask = dat ? 0xffff : 0;
		break;
/* @@1
	case 0x0200:		// �f�X�e�B�l�[�V�����u���b�N��0�̕��������]������
	case 0x0300:		// �֎~ �� 0x0200�Ɠ���
		datmask = dest ? 0 : 0xffff;
		break;
*/
	}
/*
	sgp.dest.buf = (sgp.dest.buf << 4) | dat;
	sgp.dest.mask = (sgp.dest.mask << 4) | (dat ? 0x000f : 0);
*/
	sgp.newval = (sgp.newval << BPP) | (dat & PIXMASK);
	sgp.newvalmask = (sgp.newvalmask << BPP) | (datmask & PIXMASK);
	sgp.dest.dotcount--;
//@@1	sgp.dest.buf <<= BPP;

	if (EXTPIX) {
		sgp.src.buf <<= 1;
	}
	else {
		sgp.src.buf <<= BPP;
	}

	sgp.src.dotcount--;

	sgp.dest.xcount--;
	sgp.src.xcount--;

	if (sgp.dest.dotcount == 0 || sgp.dest.xcount == 0) {
/*
		sgp.dest.buf <<= sgp.dest.dotcount * 4;
		sgp.dest.mask <<= sgp.dest.dotcount * 4;
*/
		sgp.newval <<= sgp.dest.dotcount * BPP;
		sgp.newvalmask <<= sgp.dest.dotcount * BPP;
		if (EXTPIX) {
			sgp.newval &= SWAPWORD(sgp.color);
		}
//		write_word(&sgp.dest);
//@@1	write_dest();
		write_dest2();	//@@1

		sgp.dest.nextaddress += 2;
//@@1	read_word(&sgp.dest);
		sgp.dest.dotcount = dotcountmax[sgp.dest.scrnmode]; //@@1
		if ((sgp.bltmode & SGP_BLTMODE_TP) == 0x0100) {
			sgp.remainclock -= 10 * 2;
		}
		else {
			sgp.remainclock -= 8 * 2;
		}
	}

	if (sgp.dest.xcount == 0) {
		sgp.dest.ycount--;
		sgp.src.ycount--;
		if (sgp.dest.ycount == 0) {
			//sgp.func = fetch_command;
			sgp.func = FUNC_FETCH_COMMAND;
		}
		else {
			sgp.remainclock -= 14 * 2;

			if (sgp.bltmode & SGP_BLTMODE_VD) {
				sgp.src.lineaddress -= (SINT32)sgp.src.fbw;
				sgp.dest.lineaddress -= (SINT32)sgp.dest.fbw;
			}
			else {
				sgp.src.lineaddress += (SINT32)sgp.src.fbw;
				sgp.dest.lineaddress += (SINT32)sgp.dest.fbw;
			}
			if (sgp.src.ycount == 0) {
				// PATBLT�ŁAdest�̍�����src���傫�������ꍇ�ŁA
				// �������b�v�A���E���h�����������ꍇ
				init_block(&sgp.src);
			}
			sgp.src.nextaddress = sgp.src.lineaddress;
			sgp.dest.nextaddress = sgp.dest.lineaddress;
			init_src_line();
			init_dest_line();
		}
	}
	else if (sgp.src.xcount == 0) {
		// PATBLT�ŁAdest�̕���src���傫�������ꍇ�ŁA
		// �������b�v�A���E���h�����������ꍇ
		sgp.src.nextaddress = sgp.src.lineaddress;
		init_src_line();
	}
}

// HD = 1 �̏ꍇ
static void exec_bitblt_hd(void) {
	UINT16 dat;
//@@1	UINT16 dest;
	UINT16 datmask;
	int BPP = bpp[sgp.dest.scrnmode];
	UINT16 PIXMASK = ~(0xffff << BPP);
	BOOL EXTPIX = sgp.src.scrnmode == 0 && sgp.dest.scrnmode != 0;	// 1bpp����̊g���]��

	if (sgp.src.dotcount == 0) {
		read_word(&sgp.src);
		sgp.src.nextaddress -= 2;
	}
	if (EXTPIX) {
		dat = (sgp.src.buf & 0x0001) ? 0xffff : 0;
	}
	else {
		dat = sgp.src.buf;
	}
//@@1	dest = sgp.dest.buf & PIXMASK;

	switch (sgp.bltmode & SGP_BLTMODE_TP) {
	case 0x0000:		// �\�[�X�����̂܂ܓ]��
	default:	//@@1
		datmask = 0xffff;
		break;
	case 0x0100:		// �\�[�X��0�̕����͓]�����Ȃ�
		datmask = dat ? 0xffff : 0;
		break;
/* @@1
	case 0x0200:		// �f�X�e�B�l�[�V�����u���b�N��0�̕��������]������
	case 0x0300:		// �֎~ �� 0x0200�Ɠ���
		datmask = dest ? 0 : 0xffff;
		break;
*/
	}

	sgp.newval = (sgp.newval >> BPP) | ((dat & PIXMASK) << (16 - BPP));
	sgp.newvalmask = (sgp.newvalmask >> BPP) | ((datmask & PIXMASK) << (16 - BPP));
	sgp.dest.dotcount--;
//@@1	sgp.dest.buf >>= BPP;

	if (EXTPIX) {
		sgp.src.buf >>= 1;
	}
	else {
		sgp.src.buf >>= BPP;
	}

	sgp.src.dotcount--;

	sgp.dest.xcount--;
	sgp.src.xcount--;

	if (sgp.dest.dotcount == 0 || sgp.dest.xcount == 0) {

		sgp.newval >>= sgp.dest.dotcount * BPP;
		sgp.newvalmask >>= sgp.dest.dotcount * BPP;
		if (EXTPIX) {
			sgp.newval &= SWAPWORD(sgp.color);
		}
//@@1		write_dest();
		write_dest2();	//@@1
		sgp.dest.nextaddress -= 2;
//@@1		read_word(&sgp.dest);
		sgp.dest.dotcount = dotcountmax[sgp.dest.scrnmode];	//@@1
		if ((sgp.bltmode & SGP_BLTMODE_TP) == 0x0100) {
			sgp.remainclock -= 10 * 2;
		}
		else {
			sgp.remainclock -= 8 * 2;
		}
	}

	if (sgp.dest.xcount == 0) {
		sgp.dest.ycount--;
		sgp.src.ycount--;
		if (sgp.dest.ycount == 0) {
			sgp.func = FUNC_FETCH_COMMAND;
		}
		else {
			sgp.remainclock -= 14 * 2;

			if (sgp.bltmode & SGP_BLTMODE_VD) {
				sgp.src.lineaddress -= (SINT32)sgp.src.fbw;
				sgp.dest.lineaddress -= (SINT32)sgp.dest.fbw;
			}
			else {
				sgp.src.lineaddress += (SINT32)sgp.src.fbw;
				sgp.dest.lineaddress += (SINT32)sgp.dest.fbw;
			}
			if (sgp.src.ycount == 0) {
				// PATBLT�ŁAdest�̍�����src���傫�������ꍇ�ŁA
				// �������b�v�A���E���h�����������ꍇ
				init_block(&sgp.src);
			}
			sgp.src.nextaddress = sgp.src.lineaddress;
			sgp.dest.nextaddress = sgp.dest.lineaddress;
			init_src_line_hd();
			init_dest_line_hd();
		}
	}
	else if (sgp.src.xcount == 0) {
		// PATBLT�ŁAdest�̕���src���傫�������ꍇ�ŁA
		// �������b�v�A���E���h�����������ꍇ
		sgp.src.nextaddress = sgp.src.lineaddress;
		init_src_line_hd();
	}
}


static void exec_cls(void) {
	sgp_memorywrite_w(sgp.clsaddr, sgp.color);
	sgp.clsaddr += 2;
	sgp.clscount--;
	if (sgp.clscount == 0) {
		sgp.func = FUNC_FETCH_COMMAND;
	}
	sgp.remainclock -= 3 * 2;
}

/*
static void write_line_word(void) {
	UINT16 dat;
	UINT16 dest;
	UINT16 datmask;

	dest = sgp_memoryread_w(sgp.dest.nextaddress);
	datmask = sgp.newvalmask;
	dat = logicalop(sgp.newval, dest, &datmask);

	dat = (dest & ~datmask) | (dat & datmask);
	dat = SWAPWORD(dat);
	sgp_memorywrite_w(sgp.dest.nextaddress, dat);
}
*/

/*
X������1�h�b�g���i�߂Ȃ��烉�C����`�悷��
*/
static void exec_line_x(void) {
	int xdir, ydir;
	int shift;
//	BOOL written = FALSE;
	UINT16 dat;
//	UINT16 dest;
	UINT16 datmask;
	int DOTCOUNTMAX = dotcountmax[sgp.dest.scrnmode];
	int BPP = bpp[sgp.dest.scrnmode];
//	UINT16 PIXMASK = ~(0xffff << BPP);			// 4bpp�Ȃ� 0x000f
	UINT16 PIXMASK;

	xdir = (sgp.bltmode & SGP_BLTMODE_LINE_HD) ? -1 : 1;
	ydir = (sgp.bltmode & SGP_BLTMODE_LINE_VD) ? -1 : 1;

	// ���݈ʒu�Ƀh�b�g��`��
	dat = SWAPWORD(sgp.color);
//	dat >>= (sgp.dest.dotcount - 1) * BPP;
//	dat &= PIXMASK;
//	dest = sgp.dest.buf >> (16 - BPP);

	switch (sgp.bltmode & SGP_BLTMODE_TP) {
	case 0x0000:		// �\�[�X�����̂܂ܓ]��
	default:
		datmask = 0xffff;
		break;
	case 0x0100:		// �\�[�X��0�̕����͓]�����Ȃ�
//		datmask = dat ? 0xffff : 0;
		datmask = zeromask(dat, sgp.dest.scrnmode);
		break;
//	case 0x0200:		// �f�X�e�B�l�[�V�����u���b�N��0�̕��������]������
//	case 0x0300:		// �֎~ �� 0x0200�Ɠ���
//		datmask = dest ? 0 : 0xffff;
//		break;
	}

	if (xdir > 0) {
		PIXMASK = ~(0xffff << BPP);			// 4bpp�Ȃ� 0x000f
		shift = (sgp.dest.dotcount - 1) * BPP;
		sgp.newval = (sgp.newval << BPP) | ((dat >> shift) & PIXMASK);
		sgp.newvalmask = (sgp.newvalmask << BPP) | ((datmask >> shift) & PIXMASK);
//		sgp.dest.buf <<= BPP;
	}
	else {
		PIXMASK = ~(0xffff >> BPP);			// 4bpp�Ȃ� 0xf000
		shift = (sgp.dest.dotcount - 1) * BPP;
		sgp.newval = (sgp.newval >> BPP) | ((dat << shift) & PIXMASK);
		sgp.newvalmask = (sgp.newvalmask >> BPP) | ((datmask << shift) & PIXMASK);
	}

	// ���̈ʒu�����߂�

	// x++
	sgp.dest.dotcount--;
	// y����
	sgp.lineslopecount += sgp.lineslopenumerator;
	// �J�E���^�X�V
	sgp.dest.xcount--;

	// ���ԏ���
	sgp.remainclock -= 11;	// X������1�h�b�g�i�񂾕�

	if (sgp.dest.dotcount == 0 || 
		sgp.lineslopecount >= sgp.lineslopedenominator ||
		sgp.dest.xcount == 0) {

		// ��������
		shift = sgp.dest.dotcount * BPP;
		if (xdir > 0) {
			sgp.newval <<= shift;
			sgp.newvalmask <<= shift;
		}
		else {
			sgp.newval >>= shift;
			sgp.newvalmask >>= shift;
		}

/*
		dest = sgp_memoryread_w(sgp.dest.nextaddress);
		dest = SWAPWORD(dest);
		datmask = sgp.newvalmask;
		switch (sgp.bltmode & SGP_BLTMODE_TP) {
		case 0x0200:		// �f�X�e�B�l�[�V�����u���b�N��0�̕��������]������
		case 0x0300:		// �֎~ �� 0x0200�Ɠ���
			datmask &= ~zeromask(dest, sgp.dest.scrnmode);
			break;
		}
		dat = logicalop(sgp.newval, dest, &datmask);

		dat = (dest & ~datmask) | (dat & datmask);
		dat = SWAPWORD(dat);
		sgp_memorywrite_w(sgp.dest.nextaddress, dat);
*/
		write_dest2();

		// ���ԏ���
		sgp.remainclock -= 3;	// 1���[�h�������񂾕�

		if (sgp.dest.xcount > 0) {
			if (sgp.dest.dotcount == 0) {
				// �A�h���X��2�i�߂�
				if (xdir > 0) {
					sgp.dest.nextaddress += 2;
				}
				else {
					sgp.dest.nextaddress -= 2;
				}
				sgp.dest.dotcount = DOTCOUNTMAX;
			}
			if (sgp.lineslopecount >= sgp.lineslopedenominator) {
				sgp.lineslopecount -= sgp.lineslopedenominator;
				// y = y + ydir
				if (ydir > 0) {
					sgp.dest.nextaddress += sgp.dest.fbw;
				}
				else {
					sgp.dest.nextaddress -= sgp.dest.fbw;
				}

				// ���ԏ���
				sgp.remainclock -= 11;	// Y������1�h�b�g�i�񂾕�
			}

//			dest = sgp_memoryread_w(sgp.dest.nextaddress);
//			sgp.dest.buf = SWAPWORD(dat);
//			sgp.dest.buf <<= (DOTCOUNTMAX - sgp.dest.dotcount) * BPP;
			sgp.newval = 0;
			sgp.newvalmask = 0;
		}
	}

	// �I������
	if (sgp.dest.xcount == 0) {
		sgp.func = FUNC_FETCH_COMMAND;
	}
}




/*
Y������1�h�b�g���i�߂Ȃ��烉�C����`�悷��
*/
static void exec_line_y(void) {
	static UINT32 ystepwait[]={8,9,9,10};
	int ydir, xdir;
	int DOTCOUNTMAX = dotcountmax[sgp.dest.scrnmode];
//	UINT16 dest;
	UINT16 dat;
	UINT16 datmask;
	int BPP = bpp[sgp.dest.scrnmode];
	UINT16 pixmask;

	ydir = (sgp.bltmode & SGP_BLTMODE_LINE_VD) ? -1 : 1;
	xdir = (sgp.bltmode & SGP_BLTMODE_LINE_HD) ? -1 : 1;

	// ���݈ʒu�Ƀh�b�g��`��
	dat = SWAPWORD(sgp.color);
	switch (sgp.bltmode & SGP_BLTMODE_TP) {
	case 0x0000:		// �\�[�X�����̂܂ܓ]��
	default:
		datmask = 0xffff;
		break;
	case 0x0100:		// �\�[�X��0�̕����͓]�����Ȃ�
		datmask = zeromask(dat, sgp.dest.scrnmode);
		break;
	}

	pixmask = ~(0xffff >> BPP);
	pixmask >>= sgp.dest.dotcount * BPP;
	datmask &= pixmask;

	sgp.newval = dat;
	sgp.newvalmask = datmask;

	write_dest2();

	// ���ԏ���
	sgp.remainclock -= 3;	// 1���[�h�������񂾕�

/*	
	dat = SWAPWORD(sgp.color);
	dest = sgp_memoryread_w(sgp.dest.nextaddress);
	dest = SWAPWORD(dest);

	switch (sgp.bltmode & SGP_BLTMODE_TP) {
	case 0x0000:		// �\�[�X�����̂܂ܓ]��
		datmask = 0xffff;
		break;
	case 0x0100:		// �\�[�X��0�̕����͓]�����Ȃ�
//		datmask = (dat & pixmask) ? 0xffff : 0;
		datmask = zeromask(dat, sgp.dest.scrnmode);
		break;
	case 0x0200:		// �f�X�e�B�l�[�V�����u���b�N��0�̕��������]������
	case 0x0300:		// �֎~ �� 0x0200�Ɠ���
//		datmask = (dest & pixmask) ? 0 : 0xffff;
		datmask = ~zeromask(dest, sgp.dest.scrnmode);
		break;
	}

	pixmask = ~(0xffff >> BPP);
	pixmask >>= sgp.dest.dotcount * BPP;
	datmask &= pixmask;
	dat = logicalop(dat, dest, &datmask);

	dat = (dest & ~datmask) | (dat & datmask);

	dat = SWAPWORD(dat);
	sgp_memorywrite_w(sgp.dest.nextaddress, dat);
*/

	// ���̈ʒu�����߂�
	
	// y = y + ydir
	if (ydir > 0) {
		sgp.dest.nextaddress += sgp.dest.fbw;
	}
	else {
		sgp.dest.nextaddress -= sgp.dest.fbw;
	}

	// ���ԏ���
	sgp.remainclock -= ystepwait[sgp.dest.scrnmode];	// Y������1�h�b�g�i�񂾕�

	sgp.lineslopecount += sgp.lineslopenumerator;
	if (sgp.lineslopecount >= sgp.lineslopedenominator) {
		// x = x + xdir
		sgp.dest.dotcount += xdir;
		if (sgp.dest.dotcount < 0) {
			sgp.dest.nextaddress -= 2;
			sgp.dest.dotcount += DOTCOUNTMAX;
		}
		else if (sgp.dest.dotcount >= DOTCOUNTMAX) {
			sgp.dest.nextaddress += 2;
			sgp.dest.dotcount -= DOTCOUNTMAX;
		}
		
		sgp.lineslopecount -= sgp.lineslopedenominator;

		// ���ԏ���
		sgp.remainclock -= 11;	// X������1�h�b�g�i�񂾕�
	}

	// �J�E���^�̍X�V�A�I������
	sgp.dest.ycount--;
	if (sgp.dest.ycount == 0) {
		sgp.func = FUNC_FETCH_COMMAND;
	}
}



// ---- 

typedef void (*COMMANDFUNC)();

static COMMANDFUNC commandtable[] = {
	cmd_unknown,			// 00
	cmd_end,				// 01
	cmd_nop,				// 02
	cmd_set_work,			// 03
	cmd_set_source,			// 04
	cmd_set_destination,	// 05
	cmd_set_color,			// 06
	cmd_bitblt,				// 07
	cmd_patblt,				// 08
	cmd_line,				// 09
	cmd_cls,				// 0a
	cmd_scan_right,			// 0b
	cmd_scan_left,			// 0c
//	cmd_unknown,			// 0d
//	cmd_unknown,			// 0e
//	cmd_unknown,			// 0f
};

// ---- 

static void fetch_command(void) {
	WORD cmd;
	
	cmd = sgp_memoryread_w(sgp.pc);
	sgp.pc += 2;
	if (cmd >= 0x0d) {
		TRACEOUT(("SGP: cmd: unknown %04x", cmd));
	}
	else {
		commandtable[cmd]();
	}
}

void sgp_step(void) {
	UINT32 now;
	UINT32 past;

	if (!gactrlva.gmsp) return;

	now = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
	past = now - sgp.lastclock;
	sgp.remainclock += past;

	while (sgp.remainclock > 0) {
		if (!(sgp.busy & SGP_BUSY)) {
			sgp.remainclock = 0;
			break;
		}
		//sgp.func();
		switch (sgp.func) {
		case FUNC_EXEC_BITBLT:
			exec_bitblt();
			break;
		case FUNC_EXEC_BITBLT_HD:
			exec_bitblt_hd();
			break;
		case FUNC_EXEC_CLS:
			exec_cls();
			break;
		case FUNC_EXEC_LINE_X:
			exec_line_x();
			break;
		case FUNC_EXEC_LINE_Y:
			exec_line_y();
			break;
		case FUNC_FETCH_COMMAND:
		default:
			fetch_command();
			break;
		}
	}
	sgp.lastclock = now;
}

// ---- I/O

static REG8 IOINPCALL sgp_i_notimpl(UINT port) {
	REG8 dat;

	if (port & 1) {
		// high
		// ���@�ł͕K���������肵�Ă��Ȃ�
		if (port == 0x501 || port == 0x503) {
			dat = 0xff;
		}
		else {
			dat = (port & 0x02) ? 0xfd : 0xff;
		}
	}
	else {
		// low
		// ���@�ł͕K���������肵�Ă��Ȃ�
		dat = ((port & 0x0f) == 0x0a) ? 0xfa : 0xfe;
	}

	TRACEOUT(("SGP: read unknown port %x: value=%x cs:ip=%.4x %.4x", port, dat, CPU_CS, CPU_IP));

	return dat;
}

static REG8 IOINPCALL sgp_i_notactive(UINT port) {
	REG8 dat;

	dat = ((port & 0x0f) == 0x0a) ? 0xfa : 0xfe;
	return dat;
}

/*
�v���O�����J�E���^
*/
static void IOOUTCALL sgp_o500(UINT port, REG8 dat) {
	UINT32 mask;
	int	bit;

	mask = 0x000000ffL;
	bit = (port - 0x500) * 8;
	mask <<= bit;
	sgp.initialpc = (sgp.initialpc & ~mask | ((UINT32)dat << bit)) & 0xfffffffeL;
}

/*
���荞�݋��A���f�v��
*/
static void IOOUTCALL sgp_o504(UINT port, REG8 dat) {
	dat &= SGP_INTF | SGP_ABORT;
	sgp.ctrl = dat;
	if (sgp.ctrl & SGP_ABORT) {
		sgp.busy &= ~SGP_BUSY;
		setintreq();
	}
	if (!(sgp.ctrl & SGP_INTF)) {
		if (sgp.intreq) {
			sgp.intreq = 0;
			pic_resetirq(8);
		}
	}
}

static REG8 IOINPCALL sgp_i504(UINT port) {
	if (!gactrlva.gmsp) return sgp_i_notactive(port);
	return sgp.ctrl;
}

/*
���s�J�n
*/
static void IOOUTCALL sgp_o506(UINT port, REG8 dat) {
	dat &= SGP_BUSY;
	if (!(sgp.busy & SGP_BUSY) && (dat & SGP_BUSY)) {
		// ���s�J�n
		//sgp.func = fetch_command;
		sgp.func = FUNC_FETCH_COMMAND;
		sgp.pc = sgp.initialpc;
		TRACEOUT(("SGP: start: pc=%04x", sgp.pc));
	}
	sgp.busy = dat;
}

/*
�X�e�[�^�X�ǂݏo��
*/
static REG8 IOINPCALL sgp_i506(UINT port) {
	if (!gactrlva.gmsp) return sgp_i_notactive(port);
//	TRACEOUT(("SGP: read status: %02x", sgp.busy));
	return sgp.busy;
}

/*
???
*/
static REG8 IOINPCALL sgp_i508(UINT port) {
	if (!gactrlva.gmsp) return sgp_i_notactive(port);
	TRACEOUT(("sgp: read unknown port %x: cs:ip=%.4x:%.4x", port, CPU_CS, CPU_IP));
	return  1;
}

// ---- I/F

void sgp_reset(void) {
	ZeroMemory(&sgp, sizeof(sgp));
	sgp.lastclock = CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK;
}

void sgp_bind(void) {
	int i;

	for (i = 0x500; i < 0x510; i++) {
		iocoreva_attachinp(i, sgp_i_notimpl);
	}

	for (i = 0x500; i < 0x504; i++) {
		iocoreva_attachout(i, sgp_o500);
	}
	iocoreva_attachout(0x504, sgp_o504);
	iocoreva_attachinp(0x504, sgp_i504);

	iocoreva_attachout(0x506, sgp_o506);
	iocoreva_attachinp(0x506, sgp_i506);

	iocoreva_attachinp(0x508, sgp_i508);
}

#endif
