#include	"compiler.h"
#include	"joymng.h"
#include	"soundmng.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"cbuscore.h"
#include	"board14.h"
#include	"board26k.h"
#include	"board86.h"
#include	"boardx2.h"
#include	"board118.h"
#include	"boardspb.h"
#include	"amd98.h"
#include	"pcm86io.h"
#include	"cs4231io.h"
#include	"sound.h"
#include	"fmboard.h"
#include	"beep.h"
#include	"keydisp.h"
#include	"keystat.h"

#if defined(SUPPORT_PC88VA)
#include	"boardsb2.h"
#endif

	UINT32		usesound;
	OPN_T		opn;
	AMD98		amd98;
	MUSICGEN	musicgen;

	_TMS3631	tms3631;
	_FMTIMER	fmtimer;
	_OPNGEN		opngen;
	OPNCH		opnch[OPNCH_MAX];
	_PSGGEN		psg1;
	_PSGGEN		psg2;
	_PSGGEN		psg3;
	_RHYTHM		rhythm;
	_ADPCM		adpcm;
	_PCM86		pcm86;
	_CS4231		cs4231;

#if defined(SUPPORT_PC88VA)
	_FMBOARDVA	fmboardva;
#endif


static void	(*extfn)(REG8 enable);


// ----

static	REG8	rapids = 0;

REG8 fmboard_getjoy(PSGGEN psg) {

	REG8	ret;

	rapids ^= 0xf0;											// ver0.28
	ret = 0xff;
	if (!(psg->reg.io2 & 0x40)) {
		ret &= (joymng_getstat() | (rapids & 0x30));
		if (np2cfg.KEY_MODE == 1) {
			ret &= keystat_getjoy();
		}
	}
	else {
		if (np2cfg.KEY_MODE == 2) {
			ret &= keystat_getjoy();
		}
	}
	if (np2cfg.BTN_RAPID) {
		ret |= rapids;
	}

	// rapidと非rapidを合成								// ver0.28
	ret &= ((ret >> 2) | (~0x30));

	if (np2cfg.BTN_MODE) {
		BYTE bit1 = (ret & 0x20) >> 1;					// ver0.28
		BYTE bit2 = (ret & 0x10) << 1;
		ret = (ret & (~0x30)) | bit1 | bit2;
	}

	// intr 反映して終わり								// ver0.28
	ret &= 0x3f;
	ret |= fmtimer.intr;
	return(ret);
}


// ----

void fmboard_extreg(void (*ext)(REG8 enable)) {

	extfn = ext;
}

void fmboard_extenable(REG8 enable) {

	if (extfn) {
		(*extfn)(enable);
	}
}



// ----

static void setfmregs(BYTE *reg) {

	FillMemory(reg + 0x30, 0x60, 0xff);
	FillMemory(reg + 0x90, 0x20, 0x00);
	FillMemory(reg + 0xb0, 0x04, 0x00);
	FillMemory(reg + 0xb4, 0x04, 0xc0);
}

void fmboard_reset(UINT32 type) {

	BYTE	cross;

	soundrom_reset();
	beep_reset();												// ver0.27a
	cross = np2cfg.snd_x;										// ver0.30

	extfn = NULL;
	ZeroMemory(&opn, sizeof(opn));
	setfmregs(opn.reg + 0x000);
	setfmregs(opn.reg + 0x100);
	setfmregs(opn.reg + 0x200);
	setfmregs(opn.reg + 0x300);
	opn.reg[0xff] = 0x01;
	opn.channels = 3;
	opn.adpcmmask = (UINT8)~(0x1c);

	ZeroMemory(&musicgen, sizeof(musicgen));
	ZeroMemory(&amd98, sizeof(amd98));

	tms3631_reset(&tms3631);
	opngen_reset();
	psggen_reset(&psg1);
	psggen_reset(&psg2);
	psggen_reset(&psg3);
	rhythm_reset(&rhythm);
	adpcm_reset(&adpcm);
	pcm86_reset();
	cs4231_reset();

#if defined(SUPPORT_PC88VA)
	fmboardva.sintm = 0;
#endif

	switch(type) {
		case 0x01:
			board14_reset();
			break;

		case 0x02:
			board26k_reset();
			break;

		case 0x04:
			board86_reset();
			break;

		case 0x06:
			boardx2_reset();
			break;

		case 0x08:
			board118_reset();
			break;

		case 0x14:
			board86_reset();
			break;

		case 0x20:
			boardspb_reset();
			cross ^= np2cfg.spb_x;
			break;

		case 0x40:
			boardspr_reset();
			cross ^= np2cfg.spb_x;
			break;

		case 0x80:
//			amd98_reset();
			break;

#if defined(SUPPORT_PC88VA)
		case 0x0200:
			boardsb2_reset();
			break;
#endif

		default:
			type = 0;
			break;
	}
	usesound = type;
	soundmng_setreverse(cross);
	keydisp_setfmboard(type);
	opngen_setVR(np2cfg.spb_vrc, np2cfg.spb_vrl);
}

void fmboard_bind(void) {

	switch(usesound) {
		case 0x01:
			board14_bind();
			break;

		case 0x02:
			board26k_bind();
			break;

		case 0x04:
			board86_bind();
			break;

		case 0x06:
			boardx2_bind();
			break;

		case 0x08:
			board118_bind();
			break;

		case 0x14:
			board86c_bind();
			break;

		case 0x20:
			boardspb_bind();
			break;

		case 0x40:
			boardspr_bind();
			break;

		case 0x80:
			amd98_bind();
			break;

#if defined(SUPPORT_PC88VA)
		case 0x0200:
			boardsb2_bind();
			break;
#endif
	}
	sound_streamregist(&beep, (SOUNDCB)beep_getpcm);
}


// ----

void fmboard_fmrestore(REG8 chbase, UINT bank) {

	REG8	i;
const BYTE	*reg;

	reg = opn.reg + (bank * 0x100);
	for (i=0x30; i<0xa0; i++) {
		opngen_setreg(chbase, i, reg[i]);
	}
	for (i=0xb7; i>=0xa0; i--) {
		opngen_setreg(chbase, i, reg[i]);
	}
	for (i=0; i<3; i++) {
		opngen_keyon(chbase + i, opngen.keyreg[chbase + i]);
	}
}

void fmboard_rhyrestore(RHYTHM rhy, UINT bank) {

const BYTE	*reg;

	reg = opn.reg + (bank * 0x100);
	rhythm_setreg(rhy, 0x11, reg[0x11]);
	rhythm_setreg(rhy, 0x18, reg[0x18]);
	rhythm_setreg(rhy, 0x19, reg[0x19]);
	rhythm_setreg(rhy, 0x1a, reg[0x1a]);
	rhythm_setreg(rhy, 0x1b, reg[0x1b]);
	rhythm_setreg(rhy, 0x1c, reg[0x1c]);
	rhythm_setreg(rhy, 0x1d, reg[0x1d]);
}



#if defined(SUPPORT_PC88VA)
void fmboard_setintmask(BYTE mask) {
	mask &= 0x80;
	if ((fmboardva.sintm ^ mask) & 0x80) {
		if (mask) {
			// マスク
//			TRACEOUT(("fmboard: SINTM set: reset irq"));
			pic_resetirq(fmtimer.irq);
		}
		else {
			// マスク解除
//			TRACEOUT(("fmboard: SINTM reset: fmtimer.status = %02x", fmtimer.status));
			if (fmtimer.status & 0x03) {
				// ToDo: ADPCM関連の割り込みに未対応。see FMTIMER.C fmport_a, fmport_b
//				TRACEOUT(("fmboard: set irq"));
				pic_setirq(fmtimer.irq);
				// CPU実行のループを抜けてpicの処理に移らせるため、CPU_REMCLOCK を 0にする
				/*
				if (CPU_REMCLOCK > 0) {
					CPU_BASECLOCK -= CPU_REMCLOCK;
					CPU_REMCLOCK = 0;
				}
				*/
				nevent_forceexit();
			}
		}
	}
	fmboardva.sintm = mask;
}

BYTE fmboard_getintmask(void) {
	return fmboardva.sintm;
}
#endif
