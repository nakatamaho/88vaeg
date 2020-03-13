#include	"compiler.h"
#include	"pccore.h"
#include	"diskdrv.h"
#include	"fdd_mtr.h"
#include	"timing.h"


#define	MSSHIFT		16

typedef struct {
	UINT32	tick;			// �O��timing_getcount���s����GETTICK()�̒l
	UINT32	msstep;			// 1msec������̉�ʕ\���T�C�N���� << MSSHIFT
	UINT	cnt;			// �o�ߎ��Ԃ���ʕ\���T�C�N�����ł���킵������(������)
	UINT32	fraction;		// �o�ߎ��Ԃ���ʕ\���T�C�N�����ł���킵������(�����_�ȉ�MSSHIFT�r�b�g)
} TIMING;

static	TIMING	timing;


void timing_reset(void) {

	timing.tick = GETTICK();
	timing.cnt = 0;
	timing.fraction = 0;
}

/*
�\��������ݒ肷��
	IN:		lines		1��ʂ�����̃��C����(��\����ԁA����������Ԃ��܂�)
			crthz		1�b����`�惉�C����
*/
void timing_setrate(UINT lines, UINT crthz) {

	timing.msstep = (crthz << (MSSHIFT - 3)) / lines / (1000 >> 3);
}

void timing_setcount(UINT value) {

	timing.cnt = value;
}

/*
�o�ߎ��Ԃ���ʕ\���T�C�N�����ŕԋp����
	���̒l��timing_setcount�Ń��Z�b�g�ł���B
*/
UINT timing_getcount(void) {

	UINT32	ticknow;
	UINT32	span;
	UINT32	fraction;

	ticknow = GETTICK();
	span = ticknow - timing.tick;
	if (span) {
		timing.tick = ticknow;
		fddmtr_callback(ticknow);

		if (span >= 1000) {
			span = 1000;
		}
		fraction = timing.fraction + (span * timing.msstep);
		timing.cnt += fraction >> MSSHIFT;
		timing.fraction = fraction & ((1 << MSSHIFT) - 1);
	}
	return(timing.cnt);
}

