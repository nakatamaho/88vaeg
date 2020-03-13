/*
 * VIDEOVA.H: PC-88VA Video Control
 */

enum {
	VIDEOVA_PALETTES = 32, 
	VIDEOVA_FRAMEBUFFERS = 4,

	VIDEOVA_TEXTSCREEN = 0,
	VIDEOVA_SPRITESCREEN = 1,
	VIDEOVA_GRAPHICSCREEN0 = 2,
	VIDEOVA_GRAPHICSCREEN1 = 3,
	VIDEOVA_SCREENS = 4,

	VIDEOVA_PALETTE_SCREENS = 4,				// �p���b�g�����ʂɎw��\�ȉ�ʐ�
	VIDEOVA_RGB_SCREENS		= 2,				// ���ڐF�w���ʂɎw��\�ȉ�ʐ�

	VIDEOVA_15_98KHZ		= 0,
	VIDEOVA_15_73KHZ		= 1,
	VIDEOVA_24_8KHZ			= 2,
};

typedef struct {
	DWORD	fsa;			// �X�^�[�g�A�h���X (����2bit=0,#1�ɂ͐ݒ�s��)
	WORD	fbw;			// ���� (����2bit=0)
	WORD	fbl;			// �c�� (#1�ɂ͐ݒ�s��)
	WORD	dot;			// �h�b�g�A�h���X 
	WORD	ofx;			// ���\����ʂ�X�I�t�Z�b�g(10bit,����2bit=0, #1�ɂ͐ݒ�s��)
	WORD	ofy;			// ���\����ʂ�Y�I�t�Z�b�g(10bit,#1�ɂ͐ݒ�s��)
	DWORD	dsa;			// ���\����ʂ̕\���J�n�A�h���X(����2bit=0)
	WORD	dsh;			// �T�u��ʍ��� (9bit)
	WORD	dsp;			// �T�u��ʕ\���ʒu (9bit)
} _FRAMEBUFFER, *FRAMEBUFFER;

typedef struct {
	BYTE	txtmode8;		// 030h �e�L�X�g����|�[�g
	BYTE	dmy1;
	WORD	grmode;			// 100h �\����ʐ��䃌�W�X�^
	WORD	grres;			// 102h �O���t�B�b�N��ʐ��䃌�W�X�^
	WORD	colcomp;		// 106h �p���b�g�w���ʐ��䃌�W�X�^
	WORD	rgbcomp;		// 108h ���ڐF�w���ʐ��䃌�W�X�^
	WORD	mskmode;		// 10ah ��ʃ}�X�N���[�h���W�X�^
	WORD	palmode;		// 10Ch �p���b�g���[�h���W�X�^
	WORD	dropcol;		// 10Eh �o�b�N�h���b�v�J���[
	WORD	pagemsk;		// 110h �J���[�R�[�h/�v���[���}�X�N���W�X�^
	WORD	xpar_g0;		// 124h �O���t�B�b�N���0�����F���W�X�^
	WORD	xpar_g1;		// 126h �O���t�B�b�N���1�����F���W�X�^
	WORD	xpar_txtspr;	// 12eh �e�L�X�g/�X�v���C�g�����F���W�X�^
	WORD	mskleft;		// 130h ��ʃ}�X�N�p�����[�^
	WORD	mskrit;			// 132h
	WORD	msktop;			// 134h
	WORD	mskbot;			// 136h
	BYTE	txtmode;		// 148h �e�L�X�g����|�[�g
	BYTE	dmy2;
	WORD	palette[VIDEOVA_PALETTES];

	BYTE	crtmode;		// dip sw1�̒l(���Z�b�g���ɓǂݎ��) 1..24KHz, 0..15KHz
	BYTE	dmy3;

	BYTE	dmy4[64];

	UINT16	blinkcnt;		// 1�T�C�N�����ƂɃC���N�������g�����J�E���^
	_FRAMEBUFFER	framebuffer[VIDEOVA_FRAMEBUFFERS];

	BYTE	dmy5[128];
} _VIDEOVA, *VIDEOVA;



#ifdef __cplusplus
extern "C" {
#endif

extern	_VIDEOVA videova;

void videova_reset(void);
void videova_bind(void);

int  videova_hsyncmode(void);

#ifdef __cplusplus
}
#endif

