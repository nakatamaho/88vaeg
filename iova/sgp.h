/*
 * SGP.H: PC-88VA Super Graphic Processor
 *
 */

typedef struct {
	int		scrnmode;	// �s�N�Z���T�C�Y
	int		dot;		// �J�n�h�b�g�ʒu
	UINT16	width;		// �u���b�N�̕�(�s�N�Z��, 12bit)
	UINT16	height;		// �u���b�N�̍���(�s�N�Z��, 12bit)
	SINT16	fbw;		// �t���[���o�b�t�@�̉���(�o�C�g, ����2bit=0)
	UINT32	address;	// �J�n�A�h���X(����)

	UINT32	lineaddress;
	UINT32	nextaddress;
	int		dotcount;
	UINT16	buf;
	UINT16	xcount;
	UINT16	ycount;
} _SGP_BLOCK, *SGP_BLOCK;

typedef struct {
	UINT32	initialpc;
	UINT32	pc;			// �v���O�����J�E���^
	UINT32	workmem;
	UINT8	ctrl;		// bit2:���荞�݋��� bit1=���f�v��
	UINT8	busy;		// bit0:�r�W�[

	UINT8	intreq;		// ���荞�ݗv��
	UINT8	dummy;
	UINT32	lastclock;
	SINT32	remainclock;
	UINT16	color;		// SET COLOR�Ŏw�肳�ꂽ�F

	//void (*func)();
	UINT16	func;

	_SGP_BLOCK	src;
	_SGP_BLOCK	dest;
	UINT16	newval;
	UINT16	newvalmask;
	UINT16	bltmode;

	UINT32	clsaddr;	// CLS �A�h���X
	UINT32	clscount;	// CLS �c�胏�[�h��

	UINT16	lineslopedenominator;	// LINE �X���̕���
	UINT16	lineslopenumerator;		// LINE �X���̕��q
	UINT32	lineslopecount;			// LINE �`��1�h�b�g�ɕt�����q�����Z����J�E���^

	UINT8	dummy2[64];
} _SGP, *SGP;

enum {
	SGP_INTF	= 0x04,	// ���荞�݋���
	SGP_ABORT	= 0x02, // ���f�v��

	SGP_BUSY	= 0x01,	// �r�W�[

	SGP_BLTMODE_SF	= 0x1000,
	SGP_BLTMODE_VD	= 0x0800,
	SGP_BLTMODE_HD	= 0x0400,
	SGP_BLTMODE_TP	= 0x0300,
	SGP_BLTMODE_OP	= 0x000f,

	SGP_BLTMODE_LINE_VD	= 0x0400,
	SGP_BLTMODE_LINE_HD	= 0x0800,
};

#ifdef __cplusplus
extern "C" {
#endif

void sgp_step(void);

void sgp_reset(void);
void sgp_bind(void);


extern	_SGP	sgp;

#ifdef __cplusplus
}
#endif
