#include	"compiler.h"
#include	"memory.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"vram.h"


// ---- I/O

static void IOOUTCALL crtc_o70(UINT port, REG8 dat) {

	port = (port & 0x0e) >> 1;
	dat &= 0x1f;
	if (crtc.b[port] != dat) {
		crtc.b[port] = (UINT8)dat;
		gdcs.textdisp |= GDCSCRN_ALLDRAW;
	}
}

static void IOOUTCALL crtc_o7c(UINT port, REG8 dat) {

	if (grcg.chip) {
		grcg.modereg = (UINT8)dat;
		grcg.counter = 0;
		vramop.operate &= VOP_GRCGMASK;
		vramop.operate |= ((dat & 0xc0) >> 4);
		if (grcg.chip >= 2) {
			grcg.gdcwithgrcg = (dat >> 4) & 0x0c;
		}
		i286_vram_dispatch(vramop.operate);
	}
	(void)port;
}

static void IOOUTCALL crtc_o7e(UINT port, REG8 dat) {

	int		cnt;

	cnt = grcg.counter;
	grcg.tile[cnt].b[0] = (UINT8)dat;
	grcg.tile[cnt].b[1] = (UINT8)dat;
	grcg.counter = (cnt + 1) & 3;
	(void)port;
}

static REG8 IOINPCALL crtc_i7c(UINT port) {

	(void)port;
	return(grcg.modereg);
}


// ---- I/F

static const IOOUT crtco70[8] = {
				crtc_o70,	crtc_o70,	crtc_o70,	crtc_o70,
				crtc_o70,	crtc_o70,	crtc_o7c,	crtc_o7e};

static const IOINP crtci70[8] = {
				NULL,		NULL,		NULL,		NULL,
				NULL,		NULL,		crtc_i7c,	NULL};


void crtc_biosreset(void) {

	if (!(np2cfg.dipsw[0] & 0x01)) {
		crtc.reg.pl = 0;
		crtc.reg.bl = 0x0f;
		crtc.reg.cl = 0x10;
		crtc.reg.ssl = 0;
	}
	else {
		crtc.reg.pl = 0;
		crtc.reg.bl = 0x07;
		crtc.reg.cl = 0x08;
		crtc.reg.ssl = 0;
	}
	gdcs.textdisp |= GDCSCRN_ALLDRAW;
}

void crtc_reset(void) {

	ZeroMemory(&grcg, sizeof(grcg));
	ZeroMemory(&crtc, sizeof(crtc));
#if defined(SUPPORT_PC9821)
	grcg.chip = 3;							// PC-9821は EGC必須
#else
	grcg.chip = np2cfg.grcg & 3;			// GRCG動作のコピー
#endif
	crtc_biosreset();
}

void crtc_bind(void) {

	iocore_attachsysoutex(0x0070, 0x0cf1, crtco70, 8);
	iocore_attachsysinpex(0x0070, 0x0cf1, crtci70, 8);
}

