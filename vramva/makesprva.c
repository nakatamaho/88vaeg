/*
 * MAKESPRVA.C: PC-88VA Sprite
 */

#include	"compiler.h"
//#include	"cpucore.h"
//#include	"pccore.h"
//#include	"iocore.h"
//#include	"vram.h"
//#include	"scrnmng.h"
#include	"scrndraw.h"
//#include	"scrndrawva.h"
//#include	"dispsync.h"

#include	"memoryva.h"
#include	"tsp.h"
#include	"makesprva.h"

enum {
	SPRVA_SPRS		= 0x20,
	SPRVA_MAXWIDTH	= 0x400,

	TEXTSPR_WIDTH	= SPRVA_MAXWIDTH,
};

/*
typedef struct {
	UINT16	sprtable;			// �X�v���C�g����e�[�u��(TVRAM�擪����̃I�t�Z�b�g)
	//BOOL	mg;					// �c2�{�g��
} _TSP, *TSP;
*/

typedef struct {
	BOOL	sw;					// �\���X�C�b�`
	UINT16	vlines;				// �����T�C�Y(���C��)
	UINT16	yp;					// �����\���ʒu

	UINT32	spda;				// ����\���f�[�^(TVRAM�擪����̑��΃A�h���X)
} _SPRVA, *SPRVA;

typedef struct {
	UINT	screeny;			// ���ݏ������̃��X�^(��ʋ��ʂ̍��W�n��)
	UINT	y;					// ���ݏ������̃��X�^(�X�v���C�g�̍��W�n��)
	_SPRVA	_spr[SPRVA_SPRS];
} _SPRVAWORK;

//static	_TSP	tsp;
static	_SPRVAWORK	work;
//		BYTE sprraster[SURFACE_WIDTH];
		BYTE sprraster[TEXTSPR_WIDTH];
											// 1���X�^���̃s�N�Z���f�[�^
											// �e�s�N�Z���̓p���b�g�ԍ�(0�`15)

void makesprva_initialize(void) {
/*
	tsp.sprtable = 0x7e00;
*/
}



static void drawraster(SPRVA s, BYTE *sprinfo) {
	WORD d;
	int bitcount;
//	int bytecount;
	UINT16	xbytes;		// �X�v���C�g�̕�(�o�C�g)
	UINT16	xp;			// �������W
	UINT8	fg;			// �t�H�A�O���E���h�J���[
	UINT8	bg;			// �o�b�N�O���E���h�J���[
	BOOL	md;			// ���m�N�����[�h
	BYTE	*sprdata;	// �X�v���C�g�f�[�^

	d = LOADINTELWORD(sprinfo + 2);
	xbytes = ((d >> 11) + 1) * 4;
	xp = d & 0x03ff;
	md = d & 0x0400;
	d = LOADINTELWORD(sprinfo + 6);
	fg = (d & 0x00f0) >> 4;
	bg = d & 0x0008;
	sprdata = textmem + s->spda;
	s->spda += xbytes;

	if (md) {
		if (bg) {
			// ���m�N�� �w�i�F����
			for (; xbytes > 0; xbytes--) {
				d = *sprdata;
				sprdata++;
				for (bitcount = 0; bitcount < 8; bitcount++) {
					sprraster[xp] = d & 0x80 ? fg : bg;
					d <<= 1;
					xp = (xp + 1) & 0x3ff;
				}
			}
		}
		else {
			// ���m�N�� �w�i����
			for (; xbytes > 0; xbytes--) {
				d = *sprdata;
				sprdata++;
				for (bitcount = 0; bitcount < 8; bitcount++) {
					if (d & 0x80) sprraster[xp] = fg;
					d <<= 1;
					xp = (xp + 1) & 0x3ff;
				}
			}
		}
	}
	else {
		// 16�F
		for (; xbytes > 0; xbytes--) {
			d = *sprdata;
			sprdata++;
			// ToDo: 0�ȊO�̓����F�̏ꍇ�ɏd�˂������Ȃ��悤�ɂ���
			if (d & 0xf0) sprraster[xp] = d >> 4;
			xp = (xp + 1) & 0x3ff;
			if (d &= 0x0f) sprraster[xp] = (BYTE)d;
			xp = (xp + 1) & 0x3ff;
		}
	}
#if 0
	if (xp >= SURFACE_WIDTH) xp -= 0x0400;

	d = LOADINTELWORD(sprdata);
	bitcount = 0;
	bytecount = xbytes;

	if (md) {
		if (xp + xbytes * 8 <= 0) return;

		if (xp < 0) {
			int ofs;
			ofs = -xp;
			bytecount -= ofs / 8;
										// ���̎��_�ŁA���xwords>0
			sprdata += ofs / 8;
			d = *sprdata;
			bitcount = ofs % 8;
			d <<= bitcount;
			xp = 0;
		}

		if (bg) {
			/*
			for (; wordcount > 0; wordcount--) {
				for (; bitcount < 16; bitcount++) {
					sprraster[xp] = d & 0x8000 ? fg : bg;
					d <<= 1;
					xp++;
				}
				sprdata += 2;
				d = LOADINTELWORD(sprdata);
				bitcount = 0;
			}
			*/
		}
		else {
			for (; bytecount > 0; bytecount--) {
				for (; bitcount < 8; bitcount++) {
					if (d & 0x80) sprraster[xp] = fg;
					d <<= 1;
					xp++;
				}
				sprdata++;
				d = *sprdata;
				bitcount = 0;
			}
		}
	}
	else {
	}

	s->spda += xbytes;
#endif
}

void makesprva_begin(void) {
	int i;
	BYTE *sprinfo;

	work.y = 0;
	work.screeny = 0;

	sprinfo = textmem + tsp.sprtable;
	for (i = 0; i < SPRVA_SPRS; i++) {
		WORD d;

		d = LOADINTELWORD(sprinfo + 0);
		work._spr[i].sw = d & 0x0200;
		work._spr[i].vlines = ((d >> 10) + 1) * 4;
		work._spr[i].yp = d & 0x01ff;
		d = LOADINTELWORD(sprinfo + 4);

		// TSP�A�h���X��TVRAM�擪����̑��΃o�C�g�A�h���X�ɕϊ�
		if (d & 0x8000) d = (d << 1) + 0x8000;
		else d <<= 1;
		work._spr[i].spda = d;

		// �J�[�\���̓_��
		if (i == tsp.curn && tsp.be && (tsp.blinkcnt2 & 0x08)) {
			work._spr[i].sw = FALSE;
		}

		sprinfo += 8;
	}
}

void makesprva_blankraster(void) {
	ZeroMemory(sprraster, sizeof(sprraster));
}

void makesprva_raster(void) {
	int i;
	BYTE *sprinfo;
	SPRVA s;
	BOOL mg;

	mg = tsp.mg && !((tsp.syncparam[0] & 0xc0) == 0x40);

	if (!mg || (work.screeny & 1) == 0) { 

		ZeroMemory(sprraster, sizeof(sprraster));
		sprinfo = textmem + tsp.sprtable + SPRVA_SPRS * 8;
		s = &work._spr[SPRVA_SPRS];
		for (i = 0; i < SPRVA_SPRS; i++) {
			s--;
			sprinfo -= 8;

			if (s->sw && ((work.y - s->yp) & 0x01ff) < s->vlines ) {

				drawraster(s, sprinfo);
			}
		
		}

		work.y++;
	}
	work.screeny++;

}
