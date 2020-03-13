/*
 * CGROMVA.H: PC-88VA Character generator
 *
 */

typedef	struct {
	WORD	cgaddr;				// 14Ch �n�[�h�E�F�A�����R�[�h
	BYTE	cgrow;				// 14Fh ���X�^�ԍ�/�t�H���g���E
} _CGROMVA;

#ifdef __cplusplus
extern "C" {
#endif

extern	_CGROMVA	cgromva;

BYTE *cgromva_font(UINT16 hccode);
int cgromva_width(UINT16 hccode);

void cgromva_reset(void);
void cgromva_bind(void);

#ifdef __cplusplus
}
#endif

