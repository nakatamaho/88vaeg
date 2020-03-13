/*
 * BMSIO.H: I-O Bank Memory
 */

// �\���ݒ�
typedef struct {
	BOOL	enabled;		// IO�o���N���������g�p����
	UINT16	port;			// �|�[�g�ԍ�
	UINT16	portmask;		// (�\��)
	UINT8	numbanks;		// �o���N��
} _BMSIOCFG;

// ���쎞�̍\���Ə�� (STATSAVE�̑Ώ�)
typedef struct {			// MEMORY.X86���̍\���̂ɉe��
							// ���
	UINT8	nomem;			// ���ݑI������Ă���o���N�Ƀ�����������
	UINT8	bank;			// ���ݑI������Ă���o���N

	_BMSIOCFG	cfg;		// �\��
} _BMSIO, *BMSIO;

// ���[�N
typedef struct {			// MEMORY.X86���̍\���̂ɉe��
	BYTE	*bmsmem;
	UINT32	bmsmemsize;
} _BMSIOWORK;


#ifdef __cplusplus
extern "C" {
#endif

#if defined(SUPPORT_BMS)
extern	_BMSIOCFG	bmsiocfg;
extern	_BMSIO		bmsio;
extern	_BMSIOWORK	bmsiowork;
#endif

void bmsio_set(void);
void bmsio_reset(void);
void bmsio_bind(void);

#ifdef __cplusplus
}
#endif

