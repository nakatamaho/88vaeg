
#if !defined(DISABLE_SOUND)

enum {
	BEEPEVENT_MAXBIT	= 8,
	BEEPEVENT_MAX		= (1 << BEEPEVENT_MAXBIT)
};

typedef struct {
	SINT32	clock;
	int		enable;
} BPEVENT;

typedef struct {
	UINT16	cnt;
	UINT16	hz;
	int		buz;
	int		__puchi;
	BYTE	mode;
	BYTE	padding[3];

	int		low;
	int		enable;
	int		lastenable;
	SINT32	clock;
	UINT	events;
	BPEVENT	event[BEEPEVENT_MAX];
} _BEEP, *BEEP;

typedef struct {
	UINT	rate;
	UINT	vol;
	UINT	__puchibase;
	UINT	samplebase;
} BEEPCFG;


#ifdef __cplusplus
extern "C" {
#endif

extern	_BEEP		beep;

void beep_initialize(UINT rate);
void beep_deinitialize(void);
void beep_setvol(UINT vol);
void beep_changeclock(void);

void beep_reset(void);
#if defined(SUPPORT_PC88VA)
void beep_hzset(UINT16 cnt, UINT beepclock);
#else
void beep_hzset(UINT16 cnt);
#endif
void beep_modeset(void);
void beep_eventinit(void);
void beep_eventreset(void);
void beep_lheventset(int beep_low);
void beep_oneventset(void);

void SOUNDCALL beep_getpcm(BEEP bp, SINT32 *pcm, UINT count);

#ifdef __cplusplus
}
#endif

#else

#define beep_setvol(v)
#define beep_changeclock()
#if defined(SUPPORT_PC88VA)
#define beep_hzset(c,clock)
#else
#define beep_hzset(c)
#endif
#define beep_modeset()
#define beep_eventreset()
#define beep_lheventset(b)
#define beep_oneventset()

#endif

