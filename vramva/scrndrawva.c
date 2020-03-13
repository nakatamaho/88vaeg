#include	"compiler.h"
#include	"scrnmng.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"scrndraw.h"
#include	"sdrawva.h"
#include	"dispsync.h"
#include	"maketextva.h"
#include	"makesprva.h"
#include	"makegrphva.h"

#if defined(SUPPORT_PC88VA)

enum {
	COLORCODES = 0x10000,
};



		WORD	vabitmap[SURFACE_SIZE];		// ��ʍ�������(VA 16bit/pixel)

#if defined(SUPPORT_24BPP) || defined(SUPPORT_32BPP)
		RGB32		drawcolor32[COLORCODES];
#endif

#if defined(SUPPORT_16BPP)
		RGB16		drawcolor16[COLORCODES];
#endif

static	WORD rgb8to16[256];					// VA 8bit RGB��16bit RGB



static void scrndrawva_initialize(void) {
	static int	initialized = 0;

	if (initialized) return;
	initialized=1;


	ZeroMemory(vabitmap, sizeof(vabitmap));

	// VA 8bit RGB��16bit RGB �ϊ��e�[�u���̍쐬
	{
		int i, r, g, b;

		for (i = 0; i < 256; i++) {
			b = i & 3;
			r = (i >> 2) & 7;
			g = (i >> 5) & 7;
			rgb8to16[i] = (g << 13) | (g == 0 ? 0 : 0x1c00) |
				(r << 7) | (r == 0 ? 0 : 0x0060) |
				(b << 3) | (b == 0 ? 0 : 0x0007);
		}
	}
}

static void scrndrawva_makedrawcolor(void) {
	BYTE	colorlevel5[32];			// �o�̓��x���ϊ� 5bit��8bit
	BYTE	colorlevel6[64];			// �o�̓��x���ϊ� 6bit��8bit
	int i;
	int c;

	for (i=0; i<32; i++) {
		colorlevel5[i]= (i << 3) | ((i) ?  0x07 : 0);
	}
	for (i=0; i<64; i++) {
		colorlevel6[i]= (i << 2) | ((i) ?  0x03 : 0);
	}

	for (c = 0; c < COLORCODES; c++) {
#if defined(SUPPORT_24BPP) || defined(SUPPORT_32BPP)
		drawcolor32[c].d = RGB32D(
						colorlevel5[(c & 0x03e0) >> 5], 
						colorlevel6[(c & 0xfc00) >> 10],
						colorlevel5[c & 0x1f]);
#endif
#if defined(SUPPORT_16BPP)
		{
			RGB32 rgb32;
			rgb32.d = RGB32D(
							colorlevel5[(c & 0x03e0) >> 5], 
							colorlevel6[(c & 0xfc00) >> 10],
							colorlevel5[c & 0x1f]);
			drawcolor16[c] = scrnmng_makepal16(rgb32);
		}
#endif
	}
}

RGB32 scrndrawva_drawcolor32(WORD colorva16) {
#if defined(SUPPORT_24BPP) || defined(SUPPORT_32BPP)
	return drawcolor32[colorva16];
#else
	RGB32	rgb;
	return rgb;		// ToDo:
#endif
}

BYTE scrndrawva_draw(BYTE redraw) {

	BYTE		ret;
	const	SCRNSURF	*surf;
	SDRAWFNVA	sdrawfn;
	_SDRAWVA	sdraw;
	int			height;

	scrndrawva_initialize();

	ret = 0;
	surf = scrnmng_surflock();
	if (surf == NULL) {
		goto sddr_exit1;
	}

	sdrawfn = sdrawva_getproctbl(surf);
	if (sdrawfn == NULL) {
		goto sddr_exit2;
	}

	height = surf->height;

	sdraw.dst = surf->ptr;
	sdraw.width = surf->width;
	sdraw.xbytes = surf->xalign * surf->width;
	sdraw.y = 0;
	sdraw.xalign = surf->xalign;
	sdraw.yalign = surf->yalign;
	sdrawfn(&sdraw, height);

sddr_exit2:
	scrnmng_surfunlock(surf);

sddr_exit1:
	return ret;

}


void scrndrawva_redraw(void) {

	scrnmng_allflash();
	dispsync_renewalmode();
	scrndrawva_makedrawcolor();
	scrndrawva_draw(1);
}

enum {
	INSIDE	= 0,
	OUTSIDE	= 1,
};

typedef struct {
	WORD	*raster;		// 1���C�����̐F�f�[�^(�p���b�g�ԍ�orVA8bit/16bit�J���[�R�[�h)
	BOOL	mask[2];		// �}�X�N�̈�̓���([0]),�O��([1])���}�X�N
	int		pixelmode;		// �s�N�Z���T�C�Y
							// 0..1bpp, 1..4bpp, 2..8bpp, 3..16bpp

							// �p���b�g�w���ʂ̏ꍇ�̂�
	BYTE	palflip;		// �p���b�g�ԍ��� XOR����f�[�^
							// �p���b�g�Z�b�g0�g�p����0, 1�g�p����0x10
	BYTE	pixelmask;		// xxxx_raster[]����ǂݏo�����l�ɓK�p����}�X�N
							// �ʏ�0x0f, 32�F���[�h���̂�0x1f
	DWORD	xpar;			// �����F�t���O(bit0:�p���b�g0�`bit31:�p���b�g31)

} _COMPSCRN, *COMPSCRN;

typedef struct {
	WORD		*bp;			// vabitmap
	int			y;
	_COMPSCRN	scrn[VIDEOVA_PALETTE_SCREENS + VIDEOVA_RGB_SCREENS];
} _COMPOSEWORK;

static	_COMPOSEWORK	work;

static	WORD	tsptext_raster[SURFACE_WIDTH];
static	WORD	tspspr_raster[SURFACE_WIDTH];
												// 1���X�^���̃s�N�Z���f�[�^
												// �e�s�N�Z���̓p���b�g�ԍ�(0�`15)


void scrndrawva_compose_begin(void) {
	work.bp = vabitmap;
	work.y = 0;
}

void scrndrawva_compose_raster(void) {
	int	x;
	int	i;
	int palmode;
	int palset1scrn;
	int type;
	int side;
	int maskpos;
	WORD *bp;
	COMPSCRN scrn;
	BYTE palcode;
	WORD c;
	WORD tscr;
	WORD comp;
	BYTE defaultflip;
	WORD xparhigh;
	int blinkfreq;

	// �e�L�X�g�ƃX�v���C�g�̏o�͂��d�ˍ��킹�A
	// �e�L�X�g/�X�v���C�g���ʋ��E�J���[�ŕ�������
	tscr = videova.pagemsk >> 12;	// �e�L�X�g/�X�v���C�g���ʋ��E�J���[
	for (x = 0; x < SURFACE_WIDTH; x++) {
		palcode = sprraster[x];
		if (palcode == 0) palcode = textraster[x];
		if (palcode > tscr) {
			// �e�L�X�g
			tsptext_raster[x] = palcode;
			tspspr_raster[x] = 0;
		}
		else {
			// �X�v���C�g
			tsptext_raster[x] = 0;
			tspspr_raster[x] = palcode;
		}
	}

	palmode = (videova.palmode >> 6) & 3;
	defaultflip = palmode == 1 ? 0x10 : 0x00;
	palset1scrn = (videova.palmode >> 4) & 3;

	// �p���b�g�u�����N�@�\
	blinkfreq = (videova.palmode >> 2) & 3;
	if (palmode < 2 && blinkfreq > 0) {
		// �u�����Non
		// (�u�����N�̓p���b�g���[�h0,1�ł̂ݗL��)
		static UINT16 blinkontimetbl[4] = {1, 2, 4, 6};	// �|�[�g10c bit1,0���u�����N�f���[�e�B�[ n/8
		static UINT16 cyclelentbl[4] = {0, 5, 6, 7};	// 1�T�C�N�� 2^n �t���[��
		UINT16 current = (videova.blinkcnt >> (cyclelentbl[blinkfreq] - 3)) & 7;
						// blinkcnt �̑�cyclelentbl[blinkfreq] bit����3bit�擾
		if (current >= blinkontimetbl[videova.palmode & 3]) {
			// on�^�C���łȂ����p���b�g�Z�b�g�𔽓]
			defaultflip ^= 0x10;
		}
	}


	scrn = work.scrn;
	comp = videova.colcomp;
	for (i = 0; i < VIDEOVA_PALETTE_SCREENS; i++, scrn++) {
		type = comp & 0x0f;
		comp >>= 4;
		if (type < 8) {
			scrn->raster = NULL;
			/*
			// raster == NULL�̂Ƃ��́Amask==TRUE�ɂ��āA�`�悳��Ȃ��悤�ɂ���
			scrn->mask[OUTSIDE] = TRUE;
			scrn->mask[INSIDE] = TRUE;
			continue;
			*/
		}
		else {
			type &= 0x03;
			scrn->palflip = defaultflip;
			if (palmode == 2 && type == palset1scrn) scrn->palflip = 0x10;
			scrn->pixelmask = 0x0f;

			switch (type) {
			case VIDEOVA_TEXTSCREEN:
				scrn->raster = tsptext_raster;
				scrn->xpar = videova.xpar_txtspr | ((DWORD)videova.xpar_txtspr << 16);
				break;
			case VIDEOVA_SPRITESCREEN:
				scrn->raster = tspspr_raster;
				scrn->xpar = videova.xpar_txtspr | ((DWORD)videova.xpar_txtspr << 16);
				break;
			case VIDEOVA_GRAPHICSCREEN0:
				if (videova.grmode & 0x8000) {
					// GDEN0 = 1 (�O���t�B�b�N�\���C�l�[�u��)
					scrn->raster = (grph0_noraster) ? NULL : grph0_raster;
					scrn->pixelmode = videova.grres & 0x0003;
					xparhigh = videova.xpar_g0;
					if (palmode == 3 && scrn->pixelmode >= 2) {
						// 32�F���[�h
						scrn->pixelmask = 0x1f;
						xparhigh = 0x0000;
					}
					scrn->xpar = videova.xpar_g0 | ((DWORD)xparhigh << 16);
				}
				else {
					// GDEN0 = 0 (�O���t�B�b�N�\���֎~)
					scrn->raster = NULL;
				}
				break;
			case VIDEOVA_GRAPHICSCREEN1:
				if (videova.grmode & 0x8000) {
					// GDEN0 = 1 (�O���t�B�b�N�\���C�l�[�u��)
					scrn->raster = (grph1_noraster) ? NULL : grph1_raster;
					scrn->pixelmode = (videova.grres >> 8) & 0x0003;
					xparhigh = videova.xpar_g1;
					if (palmode == 3 && scrn->pixelmode >= 2) {
						// 32�F���[�h (8bpp/16bpp, �p���b�g���[�h3)
						scrn->pixelmask = 0x1f;
						xparhigh = 0x0000;
					}
					scrn->xpar = videova.xpar_g1 | ((DWORD)xparhigh << 16);
				}
				else {
					// GDEN0 = 0 (�O���t�B�b�N�\���֎~)
					scrn->raster = NULL;
				}
				break;
			}

			// screen mask
			maskpos = (videova.mskmode >> 4) & 3;
			if (i == maskpos + 1) {
				// ����̒�D����
				scrn->mask[OUTSIDE] = videova.mskmode & 0x04;
				scrn->mask[INSIDE] = videova.mskmode & 0x01;
			}
			else if (i > maskpos) {
				// ��D����
				scrn->mask[OUTSIDE] = (videova.mskmode & 0x0c) == 0x0c;
				scrn->mask[INSIDE] = (videova.mskmode & 0x03) == 0x03;
			}
			else {
				// ���D����
				scrn->mask[OUTSIDE] = (videova.mskmode & 0x0c) == 0x08;
				scrn->mask[INSIDE] = (videova.mskmode & 0x03) == 0x02;
			}
		}

		if (scrn->raster == NULL) {
			// raster == NULL�̂Ƃ��́Amask==TRUE�ɂ��āA�`�悳��Ȃ��悤�ɂ���
			scrn->mask[OUTSIDE] = TRUE;
			scrn->mask[INSIDE] = TRUE;
		}
	}

	//scrn = &work.scrn[VIDEOVA_PALETTE_SCREENS];
	comp = videova.rgbcomp;
	for (i = 0; i < VIDEOVA_RGB_SCREENS; i++, scrn++) {
		type = comp & 0x0f;
		comp >>= 4;
		if (type < 8 || type > 9) {
			scrn->raster = NULL;
			/*
			// raster == NULL�̂Ƃ��́Amask==TRUE�ɂ��āA�`�悳��Ȃ��悤�ɂ���
			scrn->mask[OUTSIDE] = TRUE;
			scrn->mask[INSIDE] = TRUE;
			continue;
			*/
		}
		else {
			type = (type & 0x01) + VIDEOVA_GRAPHICSCREEN0;

			if (videova.grmode & 0x8000) {
				// GDEN0 = 1 (�O���t�B�b�N�\���C�l�[�u��)
				switch (type) {
				case VIDEOVA_GRAPHICSCREEN0:
					scrn->raster = (grph0_noraster) ? NULL : grph0_raster;
					scrn->pixelmode = videova.grres & 0x0003;
					break;
				case VIDEOVA_GRAPHICSCREEN1:
					scrn->raster = (grph1_noraster) ? NULL : grph1_raster;
					scrn->pixelmode = (videova.grres >> 8) & 0x0003;
					break;
				}
			}
			else {
				// GDEN0 = 0 (�O���t�B�b�N�\���֎~)
				scrn->raster = NULL;
			}

			// screen mask
			maskpos = (videova.mskmode >> 4) & 3;
			if ((i + 4) == maskpos + 1) {
				// ����̒�D����
				scrn->mask[OUTSIDE] = videova.mskmode & 0x04;
				scrn->mask[INSIDE] = videova.mskmode & 0x01;
			}
			else /*if ((i + 4) > maskpos)*/ {
				// ��D����
				scrn->mask[OUTSIDE] = (videova.mskmode & 0x0c) == 0x0c;
				scrn->mask[INSIDE] = (videova.mskmode & 0x03) == 0x03;
			}
			/*
			else {
				// ���D����
				scrn->mask[OUTSIDE] = (videova.mskmode & 0x0c) == 0x08;
				scrn->mask[INSIDE] = (videova.mskmode & 0x03) == 0x02;
			}
			*/
		}

		if (scrn->raster == NULL) {
			// raster == NULL�̂Ƃ��́Amask==TRUE�ɂ��āA�`�悳��Ȃ��悤�ɂ���
			scrn->mask[OUTSIDE] = TRUE;
			scrn->mask[INSIDE] = TRUE;
		}
	}


	bp = work.bp;
	for (x = 0; x < SURFACE_WIDTH; x++) {
		if ((videova.grmode & 0x3000) == 0x3000) {
			// XVSP = 1 (�r�f�I�M���o�̓��[�h)
			// SYNCEN = 1 (���������M���o�̓��[�h)

			if (work.y < videova.msktop * 2 || work.y > videova.mskbot * 2 + 1 ||
				x < videova.mskleft || x > videova.mskrit) {
				side = OUTSIDE;
			}
			else {
				side = INSIDE;
			}
			// �p���b�g�w����
			scrn = &work.scrn[0];
			for (i = 0; i < VIDEOVA_PALETTE_SCREENS; i++, scrn++) {
				if (!scrn->mask[side] /*&& scrn->raster != NULL*/) {
					palcode = (scrn->raster[x] & scrn->pixelmask) ^ scrn->palflip;
					if ((scrn->xpar & (1 << palcode)) == 0) {
						// �s�����F
						c = videova.palette[palcode];
						goto opaque;
					}
				}
			}
			// ���ڐF�w����
			for (i = 0; i < VIDEOVA_RGB_SCREENS; i++, scrn++) {
				if (!scrn->mask[side] /*&& scrn->raster != NULL*/) {
					c = scrn->raster[x];
					switch (scrn->pixelmode) {
					case 2:
						c = rgb8to16[c];
						break;
					case 3:
						break;
					default:
						// 1bit/pixel, 4bit/pixel�ł͏�ɓ���
						c = 0;
						break;
					}
					if (c != 0) {
						// �s�����F
						goto opaque;
					}
				}
			}

			c = videova.dropcol;
		opaque:
			;
		}
		else {
			// XVSP = 0 (�r�f�I�M���֎~���[�h)
			// �o�b�N�h���b�v�J���[���܂߂ďo�͋֎~
			c = 0;
		}
		*bp = c;
		bp++;
	}

	work.bp = bp;
	work.y++;
}


#endif
