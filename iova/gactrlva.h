/*
 * GACTRLVA.H: PC-88VA GVRAM access control
 *
 */


#ifdef __cplusplus
extern "C" {
#endif

// Single Plane
typedef struct {
	UINT8	writemode;		// 0580h ���C�g���[�h���W�X�^
	UINT16	pattern[2];		// 0590h, 0592h �p�^�[�����W�X�^
	UINT8	rop[2];			// 05a0h, 05a2h ROP�R�[�h���W�X�^
} _GACTRLVA_SINGLE, *GACTRLVA_SINGLE;

// Multi Plane
typedef struct {
	UINT8	accessmode;		// 0510h �A�N�Z�X���[�h bit0 1..�g���A�N�Z�X 0..�Ɨ��A�N�Z�X
	UINT8	accessblock;	// 0512h �A�N�Z�X�u���b�N bit0 0..�u���b�N0 1..�u���b�N1
	UINT8	readplane;		// 0514h �ǂݏo���v���[�� bit0-3 0..�ǂݏo�� 1..�ǂݏo���Ȃ�
	UINT8	writeplane;		// 0516h �������݃v���[�� bit0-3 0..�������� 1..�������܂Ȃ�
	UINT8	advancedaccessmode;	// 0518h �g���A�N�Z�X���[�h
								// bit 1-0 
								//�@�@�O�@�@�@�O�@�@�@�Œ胂�[�h�i���[�h�^���C�g���X�V���Ȃ��j
								//�@�@�O�@�@�@�P�@�@�@���[�h���̂ݍX�V
								//�@�@�P�@�@�@�O�@�@�@���C�g���̂ݍX�V
								//�@�@�P�@�@�@�P�@�@�@���[�h�^���C�g���X�V
								// bit 2 0..8bit 1..16bit
								// bit 4-3 
								//     00 LU�o��
								//     01 �p�^�[�����W�X�^
								//     10 CPU
								//     11 �m�[�I�y���[�V����
								// bit 5 1..��r�ǂݏo��������
								// bit 7 1..�}���`�v���[�����[�h���䃌�W�X�^ ���[�h�֎~
	UINT8	cmpdata[4];			// 0520h, 0522h, 0524h, 0526h ��r�f�[�^���W�X�^
	UINT8	dmy;//cmpdatacontrol;	// 0528h ��r�f�[�^���W�X�^�ɐݒ� bit3-0 1..0ffh�ݒ� 0..0�ݒ�
	UINT8	pattern[4][2];		// [][0] 0530h, 0532h, 0534h, 0536h �p�^�[�����W�X�^����
								// [][1] 0540h, 0542h, 0544h, 0546h �p�^�[�����W�X�^���
	UINT8	patternreadpointer;	// 0550h �p�^�[�����W�X�^���[�h�|�C���^ bit3-0 1..��ʃo�C�g����g�p
	UINT8	patternwritepointer;// 0552h �p�^�[�����W�X�^���C�g�|�C���^ bit3-0 1..��ʃo�C�g����g�p
	UINT8	rop[4];				// 0560h,0562h,0564h,0566h ROP���W�X�^
} _GACTRLVA_MULTI, *GACTRLVA_MULTI;

typedef struct {
	UINT8	gmsp;				// �`�惂�[�h 0x10..�V���O���v���[�� 0x00..�}���`�v���[��
	_GACTRLVA_SINGLE	s;		// �V���O���v���[��
	_GACTRLVA_MULTI		m;		// �}���`�v���[��
} _GACTRLVA, *GACTRLVA;

extern	_GACTRLVA	gactrlva;


void gactrlva_reset(void);
void gactrlva_bind(void);


#ifdef __cplusplus
}
#endif
