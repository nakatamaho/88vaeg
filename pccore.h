
#include	"nevent.h"
#include	"statsave.h"

enum {
#if defined(SUPPORT_PC88VA)
	PCBASECLOCK40		= 3993600,
#endif
	PCBASECLOCK25		= 2457600,
	PCBASECLOCK20		= 1996800
};

enum {
	CPUMODE_8MHZ		= 0x20,
#if defined(SUPPORT_PC88VA)
	CPUMODE_BASE4MHZ	= 0x40,
#endif

	PCMODEL_VF			= 0,
	PCMODEL_VM			= 1,
	PCMODEL_VX			= 2,
	PCMODELMASK			= 0x3f,
	PCMODEL_PC9821		= 0x40,
	PCMODEL_EPSON		= 0x80,

	PCHDD_SASI			= 0x01,
	PCHDD_SCSI			= 0x02,
	PCHDD_IDE			= 0x04,

	PCROM_BIOS			= 0x01,
	PCROM_SOUND			= 0x02,
	PCROM_SASI			= 0x04,
	PCROM_SCSI			= 0x08,
	PCROM_BIOS9821		= 0x10,

	PCSOUND_NONE		= 0x00,

	PCCBUS_PC9861K		= 0x0001,
	PCCBUS_MPU98		= 0x0002,

#if defined(SUPPORT_PC88VA)
	PCMODEL_NOTVA		= 0,
	PCMODEL_VA1			= 1,
	PCMODEL_VA2			= 2,
#endif

};


typedef struct {
	// �G�~�����[�g���ɂ悭�Q�Ƃ����z
	UINT8	uPD72020;
	UINT8	DISPSYNC;
	UINT8	RASTER;
	UINT8	realpal;
	UINT8	LCD_MODE;
	UINT8	skipline;
	UINT16	skiplight;

	UINT8	KEY_MODE;
	UINT8	XSHIFT;
	UINT8	BTN_RAPID;
	UINT8	BTN_MODE;

	UINT8	dipsw[3];
	UINT8	MOUSERAPID;

	UINT8	calendar;
	UINT8	usefd144;
	UINT8	wait[6];

	// ���Z�b�g���Ƃ�����܂�Q�Ƃ���Ȃ��z
	OEMCHAR	model[8];
	UINT	baseclock;
	UINT	multiple;

	UINT8	memsw[8];

	UINT8	ITF_WORK;
	UINT8	EXTMEM;
	UINT8	grcg;
	UINT8	color16;
	UINT32	BG_COLOR;
	UINT32	FG_COLOR;

	UINT16	samplingrate;
	UINT16	delayms;
#if defined(SUPPORT_PC88VA)
	UINT16	SOUND_SW;
#else
	UINT8	SOUND_SW;
#endif
	UINT8	snd_x;

	UINT8	snd14opt[3];
	UINT8	snd26opt;
	UINT8	snd86opt;
	UINT8	spbopt;
	UINT8	spb_vrc;												// ver0.30
	UINT8	spb_vrl;												// ver0.30
	UINT8	spb_x;													// ver0.30

	UINT8	BEEP_VOL;
	UINT8	vol14[6];
	UINT8	vol_fm;
	UINT8	vol_ssg;
	UINT8	vol_adpcm;
	UINT8	vol_pcm;
	UINT8	vol_rhythm;

	UINT8	mpuenable;
	UINT8	mpuopt;

	UINT8	pc9861enable;
	UINT8	pc9861sw[3];
	UINT8	pc9861jmp[6];

	UINT8	fddequip;
	UINT8	MOTOR;
	UINT8	MOTORVOL;
	UINT8	PROTECTMEM;
	UINT8	hdrvacc;

#if defined(SUPPORT_PC88VA)
	UINT8	lockedkey;
#endif

	OEMCHAR	sasihdd[2][MAX_PATH];									// ver0.74
#if defined(SUPPORT_SCSI)
	OEMCHAR	scsihdd[4][MAX_PATH];									// ver0.74
#endif
	OEMCHAR	fontfile[MAX_PATH];
	OEMCHAR	biospath[MAX_PATH];
	OEMCHAR	hdrvroot[MAX_PATH];
} NP2CFG;

typedef struct {
	UINT32	baseclock;
	UINT	multiple;

	UINT8	cpumode;
	UINT8	model;
	UINT8	hddif;
	UINT8	extmem;
	UINT8	dipsw[3];		// ���Z�b�g����DIPSW
	UINT8	rom;

	UINT32	sound;
	UINT32	device;

	UINT32	realclock;

#if defined(SUPPORT_PC88VA)
	UINT8	model_va;
#endif
} PCCORE;

enum {
	COREEVENT_SHUT		= 0,
	COREEVENT_RESET		= 1,
	COREEVENT_EXIT		= 2
};


#if defined(VAEG_EXT)
typedef struct {
	void (*wait)(void);
	void (*onpause)(void);
} DEBUGCALLBACK;
#endif


#ifdef __cplusplus
extern "C" {
#endif

extern const OEMCHAR np2version[];

extern	NP2CFG	np2cfg;
extern	PCCORE	pccore;
extern	UINT8	screenupdate;
extern	int		soundrenewal;
extern	BOOL	drawframe;
extern	UINT	drawcount;
extern	BOOL	hardwarereset;

void getbiospath(OEMCHAR *path, const OEMCHAR *fname, int maxlen);
void screendisp(NEVENTITEM item);
void screenvsync(NEVENTITEM item);
#if defined(SUPPORT_PC88VA)
void screendispva(NEVENTITEM item);
void screenvsyncva(NEVENTITEM item);
//void screenvsyncva2(NEVENTITEM item);
void sysp4vsyncint(NEVENTITEM item);
void sysp4vsyncstart(NEVENTITEM item);
void sysp4vsyncend(NEVENTITEM item);
#endif

void pccore_cfgupdate(void);

void pccore_init(void);
void pccore_term(void);
void pccore_reset(void);
void pccore_exec(BOOL draw);

void pccore_postevent(UINT32 event);

#if defined(USEIPTRACE) && defined(TRACE)		// Shinra
void iptrace_out(void);
extern	int		treafter;
#endif

//@@@@@
void pccore_debugint(UINT32 no);
//@@@@@

#if defined(VAEG_EXT)
void pccore_debugsetcallback(DEBUGCALLBACK *callback);
void pccore_debugpause(BOOL pauseflag);
BOOL pccore_getdebugpause(void);
void pccore_debugsinglestep(BOOL singlstepflag);

void pccore_debugioin(BOOL word, UINT port);

#endif

#ifdef __cplusplus
}
#endif

