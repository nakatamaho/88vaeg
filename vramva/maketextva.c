/*
 * MAKETEXTVA.C: PC-88VA Text
 */
/*
ToDo:
	TEXTVA_ATR_UL	= 0x20,		// �A���_�[���C��
	TEXTVA_ATR_DWID	= 0x40,		// �_�u���E�B�h�X
	TEXTVA_ATR_DWIDC= 0x80,		// �_�u���E�B�h�X�R���g���[��

    �A�g���r���[�g���[�h2,3,4,5
*/

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
//#include	"vram.h"
#include	"scrnmng.h"
#include	"scrndraw.h"
#include	"scrndrawva.h"
//#include	"dispsync.h"

#include	"cgromva.h"
#include	"memoryva.h"
#include	"tsp.h"
#include	"videova.h"
#include	"maketextva.h"

#if defined(SUPPORT_PC88VA)

//#define	USETABLE					// �Ԃ��Ēx���Ȃ��Ă��܂��悤���B
//#define	USETABLE2					// ���ʂ��Ȃ��������B
#define	SLEEP_HACK

enum {
	TEXTVA_LINEHEIGHTMAX	= 20,
	TEXTVA_CHARWIDTH		= 8,
	TEXTVA_SURFACE_WIDTH	= 1024,		// �e�L�X�g�̍��W�n�̕�(�h�b�g)�́A1024�ł���B

	TEXTVA_FRAMES			= 4,		// ������ʂ̍ő吔

	TEXTVA_ATR_ST	= 0x01,		// �V�[�N���b�g
	TEXTVA_ATR_BL	= 0x02,		// �u�����N
	TEXTVA_ATR_RV	= 0x04,		// ���o�[�X
	TEXTVA_ATR_HL	= 0x08,		// �z���]���^�����C��(mode 1)
	TEXTVA_ATR_HL2	= 0x10,		// �z���]���^�����C��(mode 2,3)
	TEXTVA_ATR_UL	= 0x20,		// �A���_�[���C��
	TEXTVA_ATR_DWID	= 0x40,		// �_�u���E�B�h�X
	TEXTVA_ATR_DWIDC= 0x80,		// �_�u���E�B�h�X�R���g���[��
};

typedef struct {
	UINT8	bg;
	UINT8	fg;
	UINT8	attr;
} _CHARATTR, *CHARATTR;

typedef void (*SYNATTRFN)(BYTE, CHARATTR);	// �A�g���r���[�g�������[�`��

typedef struct {		// �e�L�X�g������ʐ���e�[�u��(�̃R�s�[)
	UINT16	vw;			// �t���[���o�b�t�@����(�o�C�g)
	UINT8	mode;		// �\�����[�h
	UINT8	fg;			// �t�H�A�O���E���h�J���[
	UINT8	bg;			// �o�b�N�O���E���h�J���[
	UINT8	rasteroffset;	// ���X�^�A�h���X�I�t�Z�b�g
	UINT32	rsa;		// ������ʃX�^�[�g�A�h���X(TVRAM�擪��0�Ƃ����o�C�g�A�h���X)
	UINT16	rh;			// ������ʂ̍���(���X�^) (16�ȏ�̋���)
	UINT16	rw;			// ������ʂ̉���(�h�b�g)(32�̔{���A�ݒ�l/8+2�������\�������)
	UINT16	rwchar;		// ������ʂ̉���(����) = rw / 8 + 2 
	UINT16	rxp;		// ������ʐ����J�n�ʒu(�h�b�g)
} _TEXTVAFRAME, *TEXTVAFRAME;

typedef struct {
	UINT	screeny;	// ���ݏ������̃��X�^(��ʋ��ʂ̍��W�n��)
	UINT	y;			// ���ݏ������̃��X�^(�e�L�X�g�̍��W�n��)
	UINT	raster;
	UINT	texty;
	UINT	lineheight;	// 1�s�̃��X�^��
/*
	UINT16	rsa;		// ������ʃX�^�[�g�A�h���X
	UINT16	vw;			// �t���[���o�b�t�@�̉���(�o�C�g)
	UINT16	rw;			// ������ʂ̉���(�h�b�g) 32�̔{�� ���̒l/8+2 �����\�������
	UINT16	rwchar;		// ������ʂ̉���(����) = rw / 8 + 2 
*/
	BOOL	linebitmap_ready;
#if defined(SLEEP_HACK)
	BOOL	allzero;	// �S�ċ󔒂��A���_�[���C���Ȃ����u�����N�Ȃ�����BG=0
	BOOL	sleep;		// �O��allzero�Ȃ̂ŕ\���x�~
#endif

	UINT		frameno;			// ���ݎQ�Ƃ��Ă��镪����ʂ̔ԍ�
	UINT		framelimit;			// ���̕�����ʂ̊J�n�ʒu(���X�^)
	SYNATTRFN	synattr;			// �A�g���r���[�g�������[�`��
	TEXTVAFRAME		frame;			// ���ݎQ�Ƃ��Ă��镪����ʂւ̃|�C���^	
	_TEXTVAFRAME	_frame[TEXTVA_FRAMES];
} _TEXTVAWORK;

static	_TEXTVAWORK	work;


static	BYTE linebitmap[TEXTVA_SURFACE_WIDTH * TEXTVA_LINEHEIGHTMAX];
											// �e�L�X�g1�s����bitmap

		BYTE textraster[SURFACE_WIDTH];		// 1���X�^���̃s�N�Z���f�[�^
											// �e�s�N�Z���̓p���b�g�ԍ�(0�`15)

#if defined(USETABLE)
static	DWORD font2bitmap[16][16][16];		// [fg][bg][4bit���̃r�b�g�}�b�v]
#endif

#if defined(USETABLE2)
static	DWORD font2bitmap[16];
#endif



void maketextva_initialize(void) {
#if defined(USETABLE)
	UINT8	fg;
	UINT8	bg;
	UINT8	dat;
	UINT8	fontdata;
	int		i;

	for (fg = 0; fg < 16; fg++) {
		for (bg = 0; bg < 16; bg ++) {
			for (dat = 0; dat < 0x10; dat++) {
				fontdata = dat;
				for (i = 0; i < 4; i++) {
					((BYTE *)&font2bitmap[fg][bg][dat])[i] = (fontdata & 0x08) ? fg : bg;
					fontdata <<= 1;
				}
			}
		}
	}
#endif

#if defined(USETABLE2)
	UINT8	dat;
	UINT8	fontdata;
	int		i;

	for (dat = 0; dat < 0x10; dat++) {
		fontdata = dat;
		for (i = 0; i < 4; i++) {
			((BYTE *)&font2bitmap[dat])[i] = (fontdata & 0x08) ? 0xff : 0;
			fontdata <<= 1;
		}
	}

#endif
}


// attribute mode 0
static void synattr0(BYTE attr, CHARATTR charattr) {
	charattr->bg = attr >> 4;
	charattr->fg = attr & 0x0f;
	charattr->attr = 0;
}

// attribute mode 1
static void synattr1(BYTE attr, CHARATTR charattr) {
	charattr->bg = work.frame->bg;
	charattr->fg = attr >> 4;
	charattr->attr = attr & 0x0f;
}

static void makeline(BYTE *v, UINT16 rwchar) {
	int		x;
	UINT	r;
	int		i;
	BYTE	*b;
	WORD	hccode;
	BYTE	*font;
	BYTE	fontdata;
	BYTE	attr;
	int		fontw;
	UINT	fonth;
	_CHARATTR	charattr;
	UINT8	bg;		//�o�b�N�O���E���h�J���[
	UINT8	fg;		//�t�H�A�O���E���h�J���[
	UINT8	ul;		//�A���_�[���C���̃J���[
#if defined(USETABLE2)
	DWORD	bg4;
	DWORD	fg4;
	DWORD	bitmap4;
#endif

/*
	if (rwchar > SURFACE_WIDTH / TEXTVA_CHARWIDTH) {
		rwchar = SURFACE_WIDTH / TEXTVA_CHARWIDTH;
	}
*/
	if (rwchar > TEXTVA_SURFACE_WIDTH / TEXTVA_CHARWIDTH) {
		rwchar = TEXTVA_SURFACE_WIDTH / TEXTVA_CHARWIDTH;
	}

	ZeroMemory(linebitmap, sizeof(linebitmap));
	b = linebitmap;

	for (x = 0; x < rwchar; x++) {
		hccode = LOADINTELWORD(v);
		attr = *(v + tsp.attroffset);
		v+=2;
		work.synattr(attr, &charattr);

		if (charattr.attr & TEXTVA_ATR_RV) {
			bg = charattr.fg;
			fg = charattr.bg;
		}
		else {
			bg = charattr.bg;
			fg = charattr.fg;
		}
		ul = fg;
		if (charattr.attr & TEXTVA_ATR_ST) {
			fg = bg;
				// �A���_�[���C���͕\�������(�V�[�N���b�g�ɂȂ�Ȃ�)
		}
		else if ((charattr.attr & TEXTVA_ATR_BL) && ((tsp.blinkcnt2 & 0x18) == 0x08)) {
			fg = bg;
				// �A���_�[���C���̓u�����N���Ȃ�
		}

#if defined(USETABLE2)
		fg4 = fg | ((DWORD)fg << 8) | ((DWORD)fg << 16) | ((DWORD)fg << 24); 
		bg4 = bg | ((DWORD)bg << 8) | ((DWORD)bg << 16) | ((DWORD)bg << 24); 
#endif

		if ((hccode == 0 || hccode == 0x20) && bg == 0 && 
			((charattr.attr & (TEXTVA_ATR_HL | TEXTVA_ATR_HL2)) == 0) ) {
			// �󔒁A�w�i�F0�A�A���_�[���C���Ȃ�
			b += TEXTVA_CHARWIDTH;
		}
		else {
#if defined(SLEEP_HACK)
			work.allzero = FALSE;
#endif

			font = cgromva_font(hccode);
			fontw = cgromva_width(hccode);
			fonth = (videova.txtmode & 0x04) ? 8 : 16;

			for (r = 0; r < work.lineheight; r++) {
				fontdata = *font;
				font += fontw;
				if ((charattr.attr & (TEXTVA_ATR_HL | TEXTVA_ATR_HL2)) && r == tsp.hlinepos) {
					for (i = 0; i < 8; i++) {
						b[i] = ul;
					}
				}
				else if (r < fonth) {
#if defined(USETABLE)
					((DWORD *)b)[0] = font2bitmap[fg][bg][fontdata >> 4];
					((DWORD *)b)[1] = font2bitmap[fg][bg][fontdata & 0x0f];
#else
#if defined(USETABLE2)
					bitmap4 = font2bitmap[fontdata >> 4];
					*((DWORD *)b) = bitmap4 & fg4 | ~bitmap4 & bg4;
					bitmap4 = font2bitmap[fontdata & 0x0f];
					*((DWORD *)(b + 4)) = bitmap4 & fg4 | ~bitmap4 & bg4;
#else
					for (i = 0; i < 8; i++) {
						b[i] = (fontdata & 0x80) ? fg : bg;
						fontdata <<= 1;
					}
#endif
#endif
				}
				else {
					for (i = 0; i < 8; i++) {
						b[i] = bg;
					}
				}
				b += TEXTVA_SURFACE_WIDTH;
			}
			b -= TEXTVA_SURFACE_WIDTH * work.lineheight - TEXTVA_CHARWIDTH;
		}
	}

}

/*
40���Ɋg�傷�鏈��(linebitmap�����H)
*/
static void conv40cm(UINT16 rwchar) {
	BYTE	*b;
	UINT	r;
	UINT	x;

	if (rwchar > TEXTVA_SURFACE_WIDTH / TEXTVA_CHARWIDTH) {
		rwchar = TEXTVA_SURFACE_WIDTH / TEXTVA_CHARWIDTH;
	}

	b = linebitmap;
	for (r = 0; r < work.lineheight; r++) {
		for (x = 0; x < rwchar; x++) {
			BYTE tmp[4];
			if (x & 1) {
				// 80�����Z���Ɋ�����E�������g��
				*(DWORD *)tmp = *((DWORD *)(b + 4));
			}
			else {
				// 80�����Z���ɋ����������������g��
				*(DWORD *)tmp = *(DWORD *)b;
			}
			*b++ = tmp[0];
			*b++ = tmp[0];
			*b++ = tmp[1];
			*b++ = tmp[1];
			*b++ = tmp[2];
			*b++ = tmp[2];
			*b++ = tmp[3];
			*b++ = tmp[3];
		}
		b += TEXTVA_SURFACE_WIDTH - rwchar * TEXTVA_CHARWIDTH;
	}
}

#if 0
void maketextva(void) {
	UINT	x;
	UINT	y;
	UINT	raster;
	UINT	texty;
	UINT16	rsa;		// ������ʃX�^�[�g�A�h���X
	UINT16	vw;			// �t���[���o�b�t�@�̉���(�o�C�g)
	UINT16	rw;			// ������ʂ̉���(�h�b�g) 32�̔{�� ���̒l/8+2 �����\�������
	UINT16	rwchar;		// ������ʂ̉���(����) = rw / 8 + 2 
	BYTE	*v;			// TVRAM
	BYTE	*b;			// bitmap
	BYTE	*lb;		// linebitmap
	BOOL	linebitmap_ready;

	linebitmap_ready = FALSE;
	rsa = 0;
	vw = 80*2;
	rw = 640;
	rwchar = rw / 8 + 2;
	raster = 0;
	texty = 0;
	for (y=0; y<SURFACE_HEIGHT; y++) {
		if (!linebitmap_ready) {
			v = textmem + rsa + vw * texty;
			makeline(v, rwchar);

			texty++;
			linebitmap_ready = TRUE;
		}

		b = np2_tbitmap + SURFACE_WIDTH * y;
		lb = linebitmap + SURFACE_WIDTH * raster;
		for (x = 0; x < SURFACE_WIDTH; x++) {
			*b = *lb;
			b++;
			lb++;
		}

		raster++;
		if (raster >= tsp.lineheight) {
			linebitmap_ready = FALSE;
			raster = 0;
		}
	}


}
#endif


static void selectframe(int no) {
	static SYNATTRFN synattrtbl[]={
		synattr0, synattr1, synattr0, synattr0, synattr0, synattr0, synattr0, synattr0
	};

	work.frameno = no;
	work.frame = &work._frame[no];
	work.framelimit = work.y + work.frame->rh;
	if (no == TEXTVA_FRAMES - 1) {	// �Ō�̕�����ʂȂ�
		work.framelimit = 0x1fe;	// rh �Ɏw��\�ȍő�l
	}
	work.texty = 0;
	work.raster = work.frame->rasteroffset;
	work.linebitmap_ready = FALSE;
	work.synattr = synattrtbl[work.frame->mode & 0x07];
}

void maketextva_begin(BOOL *scrn200) {
	int i;
	BYTE *fbinfo;

#if defined(SLEEP_HACK)
	if (work.allzero && !textmem_dirty && !tsp_dirty) {
		work.sleep = TRUE;
	}
	else {
		work.sleep = FALSE;
	}
	textmem_dirty = FALSE;
	tsp_dirty = FALSE;
	work.allzero = TRUE;
#endif

	work.y = 0;
	work.screeny = 0;

	work.lineheight = tsp.lineheight;
	if (work.lineheight > TEXTVA_LINEHEIGHTMAX) {
		work.lineheight = TEXTVA_LINEHEIGHTMAX;
	}

	// ������ʐ���e�[�u���̏����R�s�[
	fbinfo = textmem + tsp.texttable;
	for (i = 0; i < TEXTVA_FRAMES; i++) {
		TEXTVAFRAME f;
		WORD d;

		f = &work._frame[i];

		f->vw = LOADINTELWORD(fbinfo + 0x08) & 0x03ff;
		d = LOADINTELWORD(fbinfo + 0x0a);
		f->mode = d & 0x1f;
		f->bg = (d & 0x0f00) >> 8;
		f->fg = (d & 0xf000) >> 12;
		f->rasteroffset = fbinfo[0x0d] & 0x1f;
		f->rsa = LOADINTELWORD(fbinfo + 0x10);	// ToDo: �e�N�}�j�ł�16bit�����A18bit�ł͂Ȃ����H
		f->rh = LOADINTELWORD(fbinfo + 0x14) & 0x01fe;
		if (f->rh == 0) f->rh = 0x01fe;
		f->rw = LOADINTELWORD(fbinfo + 0x16) & 0x03ff;
		f->rwchar = f->rw / TEXTVA_CHARWIDTH + 2;
		f->rxp = LOADINTELWORD(fbinfo + 0x1a) & 0x03ff;

		fbinfo += 0x0020;
	}

	selectframe(0);

	//*scrn200 = tsp.hsync15khz && ((tsp.syncparam[0] & 0xc0) != 0x40);
	*scrn200 = (videova_hsyncmode() != VIDEOVA_24_8KHZ) && 
		       ((tsp.syncparam[0] & 0xc0) != 0x40);
}

void maketextva_blankraster(void) {
	ZeroMemory(textraster, sizeof(textraster));
}

void maketextva_raster(void) {
	UINT	x;
	BYTE	*v;			// TVRAM
	BYTE	*b;			// bitmap
	BYTE	*lb;		// linebitmap
	BYTE	*lbs;		// linebitmap ���Y���C���̍��[
	TEXTVAFRAME	f;

	if (videova.txtmode & 0x80) {
		// �e�L�X�g�\��OFF
		maketextva_blankraster();
		return;
	}

#if defined(SLEEP_HACK)
	if (work.sleep) {
		maketextva_blankraster();
		return;
	}
#endif

	if (!tsp.textmg || (work.screeny & 1) == 0) {

		if (work.y >= SURFACE_HEIGHT) return;

		while (work.y >= work.framelimit) {
			work.frameno++;
			selectframe(work.frameno);
		}

		f = work.frame;

		if (!work.linebitmap_ready) {
			v = textmem + f->rsa + f->vw * work.texty;
			makeline(v, f->rwchar);
			if (! videova.txtmode8 & 0x01) {
				// 40�����[�h
				conv40cm(f->rwchar);
			}

			work.texty++;
			work.linebitmap_ready = TRUE;
		}
	/*
		b = textraster;
		lb = linebitmap + SURFACE_WIDTH * work.raster;
		for (x = 0; x < SURFACE_WIDTH; x++) {
			*b = *lb;
			b++;
			lb++;
		}
	*/
		b = textraster;
		lbs = linebitmap + TEXTVA_SURFACE_WIDTH * work.raster;
		if (f->rxp) {
			lb = lbs + TEXTVA_SURFACE_WIDTH - f->rxp;
		}
		else {
			lb = lbs;
		}
		x = 0;
		if (f->rxp < SURFACE_WIDTH) {
			for (; x < f->rxp; x++) {
				*b = *lb;
				b++;
				lb++;
			}
			lb = lbs;
		}
		for (; x < SURFACE_WIDTH; x++) {
			*b = *lb;
			b++;
			lb++;
		}

		work.raster++;
		if (work.raster >= work.lineheight) {
			work.linebitmap_ready = FALSE;
			work.raster = 0;
		}

		work.y++;
	}
	work.screeny++;
}

#endif
