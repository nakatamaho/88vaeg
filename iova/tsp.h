/*
 * TSP.H: PC-88VA Text Sprite Processor
 *
 */

enum {
	TSP_F_LINESCHANGED	=	1,
};

typedef struct {
	BOOL	dspon;				// TSP�̕\��ON
	BOOL	spron;				// �X�v���C�g�̕\��ON

	UINT16	texttable;			// �e�L�X�g��ʐ���e�[�u��(TVRAM�擪����̃I�t�Z�b�g)
	UINT16	attroffset;			// �����R�[�h�̈�ƃA�g���r���[�g�̈�̍�
	UINT8	lineheight;			// 1�s�̃��X�^��
	UINT8	hlinepos;			// �z���]���^�����C����\�����郉�X�^
	UINT8	blink;				// �u�����N�����ƃJ�[�\���̓_�ő��x

	UINT8	curn;				// �J�[�\���Ƃ��Ďg�p����X�v���C�g�̔ԍ�
	BOOL	be;					// �J�[�\���u�����N

	UINT16	sprtable;			// �X�v���C�g����e�[�u��(TVRAM�擪����̃I�t�Z�b�g)
	BOOL	mg;					// �c2�{�g��
	UINT8	hspn;				// 1���X�^���ɓ����ɕ\���ł���X�v���C�g�̍ő吔
	BOOL	gr;					// �O���[�s���O���[�h

	UINT8	syncparam[14];		// SYNC�R�}���h�p�����[�^
	UINT16	screenlines;		// ��ʃ��C����(SYNC�R�}���h�Ŏw�肳�ꂽ����)
	BOOL	textmg;				// �e�L�X�g�c2�{�g��
//	BOOL	hsync15khz;			// true.15KHz, false.24KHz(SYNC�R�}���h�Ŏw�肳�ꂽ����)

	UINT16	flag;				// TSP_F_***

	UINT8	blinkcnt;			// �u�����N�p�J�E���^�B��ʕ\��1�T�C�N�����ƂɃf�N�������g
	UINT8	blinkcnt2;			// �u�����N�p�J�E���^�Bblinkcnt==0�ɂȂ邽�тɃC���N�������g

	UINT8	cmd;				// �R�}���h
	UINT8	status;				// �X�e�[�^�X(�|�[�g142h)
	//void	(*paramfunc)(REG8 dat);
	UINT16	paramfunc;			// ��M�p�����[�^�������[�`��

	UINT32	dispclock;			// �\�����Ԃ̎���(CPU�N���b�N��)
	UINT32	vsyncclock;			// ���������M��(VSYNC)�̏o�͎���(CPU�N���b�N��)
	UINT32	rasterclock;		// 1���X�^�\���̎���(CPU�N���b�N��)

	UINT32	sysp4vsyncextension;// TSP VSYNC�I���� �V�X�e���|�[�g4 VSYNC��1�ł���
								// ���������(CPU�N���b�N��)
	UINT32	sysp4dispclock;		// �V�X�e���|�[�g4 VSYNC �̕\������(CPU�N���b�N��)

	UINT8	vsync;				// bit5: 1.VSYNC����
	UINT8	sysp4vsync;			// bit5: 1.�V�X�e���|�[�g4 VSYNC����

	BYTE	dmy1[64];
	
								// paramfunc_generic �p

	UINT8	recvdatacnt;		// �p�����[�^�c���M�o�C�g��
	//BYTE	senddatacnt;		// �p�����[�^�c�著�M�o�C�g��
	//BYTE	*datap;				// ���ɑ��M����/��M����f�[�^
	UINT16	paramindex;			// ���ɑ���M����f�[�^�̈ʒu(parambuf��)
	BYTE	parambuf[16];		// ����M�o�b�t�@
	//void	(*endparamfunc)(void);
								// �p�����[�^��M�I�������[�`��

								// paramfunc_generic �p
	UINT16	execfunc;			// �R�}���h���s���[�`��

								// paramfunc_sprdef �p
	BYTE	sprdef_offset;

	BYTE	dmy2[128];
} _TSP, *TSP;


#ifdef __cplusplus
extern "C" {
#endif


void tsp_reset(void);
void tsp_bind(void);

void tsp_updateclock(void);

extern	_TSP	tsp;
extern	BOOL	tsp_dirty;

#ifdef __cplusplus
}
#endif
