/*
 * UPD9002.H: PC-88VA CPU port
 */

typedef struct {
	BYTE	tcks;			// �^�C�}�A�J�E���^�̓��̓N���b�N�w��
							// bit4-2 �^�C�}2-0�ւ̋����N���b�N 0..���� 1..�O��
							// bit1-0 ������@00..2 01..4 10..8 11..16
	BYTE	dmy[15];
} _UPD9002, *UPD9002;


#ifdef __cplusplus
extern "C" {
#endif

extern	_UPD9002		upd9002;

void upd9002_reset(void);
void upd9002_bind(void);

#ifdef __cplusplus
}
#endif

