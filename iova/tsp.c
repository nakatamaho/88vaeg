/*
 * TSP.C: PC-88VA Text Sprite Processor
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"memoryva.h"
#include	"timing.h"

#if defined(SUPPORT_PC88VA)

enum {
	// TSP�R�}���h
	CMD_SYNC	= 0x10,
	CMD_DSPON	= 0x12,
	CMD_DSPOFF	= 0x13,
	CMD_DSPDEF	= 0x14,
	CMD_CURDEF	= 0x15,
	CMD_SPRON	= 0x82,
	CMD_SPROFF	= 0x83,
	CMD_SPRDEF	= 0x84,
	CMD_EXIT	= 0x88,

	// �X�e�[�^�X
	STATUS_BUSY	= 0x04,
	STATUS_VB	= 0x40,

	// paramfunc
	PARAMFUNC_NOP			= 0,
	PARAMFUNC_GENERIC,
	PARAMFUNC_SPRDEF_BEGIN,
	PARAMFUNC_SPRDEF,

	// execfunc
	EXECFUNC_SYNC			= 0,
	EXECFUNC_DSPON,
	EXECFUNC_DSPDEF,
	EXECFUNC_CURDEF,
	EXECFUNC_SPRON,



};



		_TSP	tsp;
		BOOL	tsp_dirty;

static BYTE *getsprinfo(int no) {
	return textmem + tsp.sprtable + no * 8;
}

/*
�X�v���C�g�̕\����Ԃ�ύX����
*/
static void sprsw(int no, BOOL sw) {
	BYTE *sprinfo;
	WORD d;

	sprinfo = getsprinfo(no);
	d = LOADINTELWORD(sprinfo + 0);
	if (sw) {
		d |= 0x0200;
	}
	else {
		d &= ~0x0200;
	}
	STOREINTELWORD(sprinfo + 0, d);
}

/*
SYNC
*/
static void exec_sync(void) {
	int i;
	UINT16 newlines;
	//BOOL newhsync15khz;

	for (i = 0; i < 14; i++) tsp.syncparam[i] = tsp.parambuf[i];
	tsp.textmg = (tsp.syncparam[0] & 0xc0) == 0x80;
	newlines = tsp.syncparam[0x0a] | ((tsp.syncparam[0x0b] & 0x40) << 2);
	//newhsync15khz = tsp.syncparam[0x02] == 0x1c;
	if (newlines != tsp.screenlines /*|| newhsync15khz != tsp.hsync15khz*/) {
		tsp.screenlines = newlines;
		//tsp.hsync15khz = newhsync15khz;
		tsp.flag |= TSP_F_LINESCHANGED;
	}
	
//	TRACEOUT(("tsp: sync: textmg=0x%.2x, screenlines=%d, hsync=%s"
//		, tsp.textmg
//		, tsp.screenlines
//		, (tsp.hsync15khz) ? "15KHz" : "24KHz"));
	TRACEOUT(("tsp: sync: textmg=0x%.2x, screenlines=%d"
		, tsp.textmg
		, tsp.screenlines));

	tsp.status &= ~STATUS_BUSY;
}

/*
DSPON (TSP�\���J�n)
*/
static void exec_dspon(void) {
	TRACEOUT(("tsp: dspon: param=0x%.2x, 0x%.2x, 0x%.2x",
		tsp.parambuf[0], tsp.parambuf[1], tsp.parambuf[2]));

	tsp.texttable = tsp.parambuf[0] << 8;
	tsp.dspon = TRUE;

	tsp.status &= ~STATUS_BUSY;
}

/*
DSPDEF (��ʍ\���ƕ\���`��)
*/
static void exec_dspdef(void) {
	TRACEOUT(("tsp: dspdef: param=0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x",
		tsp.parambuf[0], tsp.parambuf[1], tsp.parambuf[2], 
		tsp.parambuf[3], tsp.parambuf[4], tsp.parambuf[5])); 

	tsp.attroffset = tsp.parambuf[0] + (WORD)tsp.parambuf[1] * 0x100;
	//tsp.pitch = tsp.parambuf[2] >> 4;
	tsp.lineheight = tsp.parambuf[3] + 1;
	tsp.hlinepos = tsp.parambuf[4];
	tsp.blink = tsp.parambuf[5] >> 3;		// ToDo: ���̂܂܍̗p����Ǝ�������������
											// �e�N�}�j�̌�A�H
	tsp.blinkcnt = tsp.blink;

	tsp.status &= ~STATUS_BUSY;
}

/*
CURDEF (�J�[�\���`�Ԃ̎w��)
*/
static void exec_curdef(void) {
	TRACEOUT(("tsp: curdef: param=0x%.2x",tsp.parambuf[0])); 

	tsp.curn = tsp.parambuf[0] >> 3;
	tsp.be = tsp.parambuf[0] & 0x01;
	sprsw(tsp.curn, tsp.parambuf[0] & 0x02);

	tsp.status &= ~STATUS_BUSY;
}

/*
SPRON (�X�v���C�g�̕\���J�n)
*/
static void exec_spron(void) {
	TRACEOUT(("tsp: spron: param=0x%.2x, 0x%.2x, 0x%.2x",tsp.parambuf[0], tsp.parambuf[1], tsp.parambuf[2])); 

	tsp.sprtable = tsp.parambuf[0] << 8;
	tsp.hspn = tsp.parambuf[2] >> 3;
	tsp.mg = tsp.parambuf[2] & 0x02;
	tsp.gr = tsp.parambuf[2] & 0x01;
	tsp.spron = TRUE;

	tsp.status &= ~STATUS_BUSY;
}

/*
SPRDEF (�X�v���C�g����e�[�u���ւ̏�������)
*/
// �p�����[�^2�o�C�g�ڈȍ~
static void paramfunc_sprdef(REG8 dat) {
	BYTE *mem;

	mem = textmem + tsp.sprtable + tsp.sprdef_offset;
	*mem = dat;
	tsp.sprdef_offset++;
}

// �p�����[�^1�o�C�g��
static void paramfunc_sprdef_begin(REG8 dat) {
	TRACEOUT(("tsp: sprdef: offset=0x%.2x", dat)); 

	tsp.sprdef_offset = dat;
	//tsp.paramfunc = paramfunc_sprdef;
	tsp.paramfunc = PARAMFUNC_SPRDEF;
}

/*
EXIT (�������~)
*/
static void exec_exit(void) {
	TRACEOUT(("tsp: exit")); 

	tsp.status &= ~STATUS_BUSY;
}

/*
����`�R�}���h
*/
static void exec_unknown(void) {
	TRACEOUT(("tsp: unknown cmd: 0x%.2x", tsp.cmd));
	tsp.status &= ~STATUS_BUSY;
}

/*
static void execcmd(void) {
	switch(tsp.cmd) {
	case CMD_DSPON:
		exec_dspon();
		break;
	case CMD_DSPDEF:
		exec_dspdef();
		break;
	case CMD_CURDEF:
		exec_curdef();
		break;
	case CMD_SPRON:
		exec_spron();
		break;
	default:
		TRACEOUT(("tsp: unknown cmd: 0x%.2x", tsp.cmd));
	}
}
*/

/*
static void paramfunc_nop(REG8 dat) {
}
*/

static void paramfunc_generic(REG8 dat) {
	if (tsp.recvdatacnt) {
		//*(tsp.datap++) = dat;
		tsp.parambuf[tsp.paramindex++] = dat;
		if (--tsp.recvdatacnt == 0) {
			//tsp.paramfunc = paramfunc_nop;
			tsp.paramfunc = PARAMFUNC_NOP;
			//tsp.endparamfunc();
			switch (tsp.execfunc) {
			case EXECFUNC_SYNC:
				exec_sync();
				break;
			case EXECFUNC_DSPON:
				exec_dspon();
				break;
			case EXECFUNC_DSPDEF:
				exec_dspdef();
				break;
			case EXECFUNC_CURDEF:
				exec_curdef();
				break;
			case EXECFUNC_SPRON:
				exec_spron();
				break;
			}
		}
	}
}

// ---- I/O


/*
�X�e�[�^�X�ǂݏo��
*/
static REG8 IOINPCALL tsp_i142(UINT port) {
	REG8 dat;

	dat = tsp.status | (tsp.vsync) ? STATUS_VB : 0;
	return dat;
}

/*
�������|�[�g
*/
static REG8 IOINPCALL tsp_i143(UINT port) {
	return 0xff;
}

/*
�R�}���h��������
*/
static void IOOUTCALL tsp_o142(UINT port, REG8 dat) {
	TRACEOUT(("tsp: command: 0x%.2x", dat));

	tsp.cmd = dat;
	//tsp.datap = tsp.parambuf;
	tsp.paramindex = 0;
	tsp.status |= STATUS_BUSY;
	tsp_dirty = TRUE;

	switch(dat) {
	case CMD_SYNC:
		tsp.recvdatacnt = 14;
		tsp.execfunc = EXECFUNC_SYNC;
		tsp.paramfunc = PARAMFUNC_GENERIC;
		break;
	case CMD_DSPON:
		tsp.recvdatacnt = 3;
		//tsp.endparamfunc = exec_dspon;
		tsp.execfunc = EXECFUNC_DSPON;
		//tsp.paramfunc = paramfunc_generic;
		tsp.paramfunc = PARAMFUNC_GENERIC;
		break;
	case CMD_DSPDEF:
		tsp.recvdatacnt = 6;
		//tsp.endparamfunc = exec_dspdef;
		tsp.execfunc = EXECFUNC_DSPDEF;
		//tsp.paramfunc = paramfunc_generic;
		tsp.paramfunc = PARAMFUNC_GENERIC;
		break;
	case CMD_CURDEF:
		tsp.recvdatacnt = 1;
		//tsp.endparamfunc = exec_curdef;
		tsp.execfunc = EXECFUNC_CURDEF;
		//tsp.paramfunc = paramfunc_generic;
		tsp.paramfunc = PARAMFUNC_GENERIC;
		break;
	case CMD_SPRON:
		tsp.recvdatacnt = 3;
		//tsp.endparamfunc = exec_spron;
		tsp.execfunc = EXECFUNC_SPRON;
		//tsp.paramfunc = paramfunc_generic;
		tsp.paramfunc = PARAMFUNC_GENERIC;
		break;
	case CMD_SPRDEF:
		//tsp.paramfunc = paramfunc_sprdef_begin;
		tsp.paramfunc = PARAMFUNC_SPRDEF_BEGIN;
		break;
	case CMD_EXIT:
		//tsp.paramfunc = paramfunc_nop;
		tsp.paramfunc = PARAMFUNC_NOP;
		exec_exit();
		break;
	default:
		//tsp.paramfunc = paramfunc_nop;
		tsp.paramfunc = PARAMFUNC_NOP;
		exec_unknown();
		break;
	}

	//if (tsp.recvdatacnt == 0) execcmd();
}

/*
�p�����[�^��������
*/
static void IOOUTCALL tsp_o146(UINT port, REG8 dat) {
	TRACEOUT(("tsp: parameter: 0x%.2x", dat));
/*
	if (tsp.recvdatacnt) {
		*(tsp.datap++) = dat;
		if (--tsp.recvdatacnt == 0) execcmd();
	}
*/
	tsp_dirty = TRUE;
	//tsp.paramfunc(dat);
	switch (tsp.paramfunc) {
	case PARAMFUNC_GENERIC:
		paramfunc_generic(dat);
		break;
	case PARAMFUNC_SPRDEF_BEGIN:
		paramfunc_sprdef_begin(dat);
		break;
	case PARAMFUNC_SPRDEF:
		paramfunc_sprdef(dat);
		break;
	case PARAMFUNC_NOP:
	default:
		break;
	}
}

// ---- I/F

void tsp_reset(void) {
	ZeroMemory(&tsp, sizeof(tsp));
	//tsp.paramfunc = paramfunc_nop;
	tsp.paramfunc = PARAMFUNC_NOP;
	tsp_dirty = TRUE;
						/* ���Z�b�g���� tsp_dirty=TRUE�Ƃ��邱�Ƃɂ��A
						   maketextva.c��sleep������ */
}

void tsp_bind(void) {
	if (pccore.model_va != PCMODEL_NOTVA) {
		tsp_updateclock();
	}
	/*
	iocoreva_attachout(0x152, memctrlva_o152);
	iocoreva_attachout(0x153, memctrlva_o153);
	iocoreva_attachout(0x198, memctrlva_o198);
	iocoreva_attachout(0x19a, memctrlva_o19a);
	*/
	iocoreva_attachinp(0x142, tsp_i142);
	iocoreva_attachinp(0x143, tsp_i143);
	iocoreva_attachout(0x142, tsp_o142);
	iocoreva_attachout(0x146, tsp_o146);
}

// ---- 

void tsp_updateclock(void) {
#if 0
	/*
	TSP�̎d�l�͂킩��Ȃ��̂ŁA����ɁA
	np2��GDC�̏����Ɠ����v�Z�����{����B
	*/
	UINT hs = 7;		// ���������M���̕�(����)
	UINT hfp = 9;		// �E�����̔�\�����(����)
	UINT hbp = 7;		// �������̔�\�����(����)
	UINT vs = 8;		// ���������M���̕�(���C��)
	UINT vfp = 7;		// �������̔�\�����(���C��)
	UINT vbp = 0x19 -3;	// ������̔�\�����(���C��)
	UINT lf = 400 +3;	// 1��ʂ�����\�����C����
	UINT cr = 80;		// 1�s������\��������
	UINT32 clock = 21052600 / 8;	// ����N���b�N=1�b������\���u���b�N��
						// (�u���b�N=1������1���C����)
	UINT x;
	UINT y;
	UINT cnt;
	UINT32 hclock;

	x = hfp + hbp + hs + cr + 3;	// 1�s�����葍�\��������
	y = vfp + vbp + vs + lf;		// 1��ʂ����葍�\�����C����

	hclock = clock / x;				// 1�b������\�����C����
	cnt = (pccore.baseclock * y) / hclock;	// 1��ʂ����莞��(�x�[�X�N���b�N��)
	cnt *= pccore.multiple;			// 1��ʂ����莞��(CPU�N���b�N��)
	tsp.rasterclock = cnt / y;
//	tsp.hsyncclock = (tsp.rasterclock * cr) / x;
	tsp.dispclock = tsp.rasterclock * lf;
	tsp.vsyncclock = cnt - tsp.dispclock;
	timing_setrate(y, hclock);
#else
	UINT lbl, lbr, had, rbr, rbl, hs, tbl, tbr, vad, bbr, bbl, vs;
	UINT w;
	UINT h;
	UINT cnt;
	UINT32 hclock;
	UINT32 clock;			// 1�b������\���h�b�g��
	UINT sysp4displines;	// �V�X�e���|�[�g4 VSYNC: �\�����ԃ��C����
	UINT sysp4vsyncexlines;	// �V�X�e���|�[�g4 VSYNC: TSP��VSYNC�I�����_����
							// 1�ł�����Ԃ̃��C����
	int hsyncmode;
	//UINT vaddefault, haddefault;

	lbl = tsp.syncparam[2] & 0x3f;	// ���u�����N
	lbr = tsp.syncparam[3] & 0x3f;	// ���{�[�_�[
	had = tsp.syncparam[4];			// �����\���̈�
	rbr = tsp.syncparam[5] & 0x3f;	// �E�{�[�_�[
	rbl = tsp.syncparam[6] & 0x3f;	// �E�u�����N
	hs  = tsp.syncparam[7] & 0x3f;	// ������������
	tbl = tsp.syncparam[8] & 0x3f;	// ��u�����N
	tbr = tsp.syncparam[9] & 0x3f;	// ��{�[�_�[
	vad = tsp.syncparam[10] + ((tsp.syncparam[11] & 0x40) << 2);
									// �����\���̈�
	bbr = tsp.syncparam[11] & 0x3f;	// ���{�[�_�[
	bbl = tsp.syncparam[12] & 0x3f;	// ���u�����N
	vs  = tsp.syncparam[13] & 0x3f;	// ������������


	if (vad == 0 && had == 0) {
		// ��������
		// SYNC�p�����^���ݒ肳��Ă��Ȃ��ꍇ�ł�tsp.dispclock/vsynclock��
		// ���������̒l�ɐݒ肳���悤�ɂ���B
		// 24KHz, 400���C���̃f�[�^��^���邱�Ƃɂ���B
		lbl = 0x10; lbr = 0;
		had = 0x9f;
		rbl = 0x10; rbr = 0;
		hs = 0x0f;
		tbl = 0x19; tbr = 0;
		vad = 0x190;
		bbl = 0x07; bbr = 0;
		vs = 8;
		hsyncmode = VIDEOVA_24_8KHZ;
	}
	else {
		hsyncmode = videova_hsyncmode();
	}

	switch(hsyncmode) {
	case VIDEOVA_24_8KHZ:
		clock = 20854022;
		sysp4displines = 402;
		sysp4vsyncexlines = 25;
		//vaddefault = 0x190;
		break;
	case VIDEOVA_15_73KHZ:
		//clock = 14219920;			// 15.73KHz
		clock = 14252364;			// ���@����l�ɋ߂��Ȃ�悤�ɕ␳:15.766KHz
		sysp4displines = 202;
		sysp4vsyncexlines = 36;
		//vaddefault = 0xc8;
		break;
	case VIDEOVA_15_98KHZ:
		clock = 14189837;
		sysp4displines = 202;
		sysp4vsyncexlines = 37;
		//vaddefault = 0xc8;
		break;
	}

	if (vs < 4) vs = 4;
	if (vad < 4) vad = 4;
	had |= 1;
	if (hs < 4) hs = 4;
	if (lbl < 3) lbl = 3;

	w = (lbl+1 + lbr + had+1 + rbr + rbl+1 + hs+1) * 4;
									// 1���C�������葍�h�b�g��
									// ��PD72022�f�[�^�V�[�g�̉���ƈقȂ�A
									// lbr, rbr�ɂ�1�����Z����Ȃ��悤��
									// ���@�ł͌�����
	h = tbl + tbr + vad + bbr + bbl + vs;
									// 1��ʂ����葍���C����

	if (sysp4displines + sysp4vsyncexlines >= h) {
		// ����VA�͋N������24KHz���ł�15.98KHz200���C���p��SYNC�ݒ������B
		// ���̏ꍇ�Asysp4displines���S���C�����������Ă��܂��|�[�g40h��
		// VSYNC�����0�ƂȂ��Ă��܂��B�����������邽�߂Ɉȉ��̏�����
		// ���s����B
		sysp4displines = h - sysp4vsyncexlines - 4;
	}


	hclock = clock / w;				// 1�b������\�����C����
	cnt = (pccore.baseclock * h) / hclock;
									// 1��ʂ����莞��(�x�[�X�N���b�N��)
	cnt *= pccore.multiple;			// 1��ʂ����莞��(CPU�N���b�N��)
	tsp.rasterclock = cnt / h;
	//tsp.dispclock = tsp.rasterclock * (tbl + tbr + vad);
	//tsp.vsyncclock = cnt - tsp.dispclock;
	tsp.vsyncclock = cnt * (bbr + bbl + vs) / h;
	tsp.dispclock = cnt - tsp.vsyncclock;
	timing_setrate(h, hclock);

//	tsp.sysp4vsyncextension = tsp.rasterclock * sysp4vsyncexlines;
//	tsp.sysp4dispclock = tsp.rasterclock * sysp4displines;
	tsp.sysp4vsyncextension = cnt * sysp4vsyncexlines / h;
	tsp.sysp4dispclock = cnt * sysp4displines / h;

#endif

}

#endif
