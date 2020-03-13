/*
 *	BOARDSB2.H: PC-88VA Sound board 2
 */

typedef	struct {
	UINT32	addrwritewait;		// ���W�X�^�A�h���X���C�g���̃E�F�C�g����(�N���b�N)
	UINT32	datawritewait;		// �f�[�^���C�g���̃E�F�C�g����(�N���b�N)

	UINT32	lastaccess;			// �Ō��write��������
	UINT32	wait;				// lastaccess����wait���Ԃ̊Ԃɔ��������A�N�Z�X��
								// �E�F�C�g����
								// 0�̏ꍇ�E�F�C�g���Ȃ�

	BOOL	waitenabled;		// �E�F�C�g�L��
} _BOARDSB2;

#ifdef __cplusplus
extern "C" {
#endif

extern	_BOARDSB2	boardsb2;

void boardsb2_reset(void);
void boardsb2_bind(void);

#ifdef __cplusplus
}
#endif

