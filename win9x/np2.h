
typedef struct {
	BYTE	port;
	BYTE	def_en;
	BYTE	param;
	UINT32	speed;
	char	mout[MAXPNAMELEN];
	char	min[MAXPNAMELEN];
	char	mdl[64];
	char	def[MAX_PATH];
} COMCFG;

typedef struct {
	char	titles[256];
#if defined(SUPPORT_PC88VA)
	char	winid[6];
#else
	char	winid[4];
#endif

	int		winx;
	int		winy;
	UINT	paddingx;
	UINT	paddingy;
	BYTE	force400;
	BYTE	WINSNAP;
	BYTE	NOWAIT;
	BYTE	DRAW_SKIP;

	BYTE	background;		// bit0..ウィンドウinactive時にエミュレーション停止
	BYTE	DISPCLK;
	BYTE	KEYBOARD;
	BYTE	F12COPY;
#if defined(VAEG_EXT)
	BYTE	ALTRKEY;
#endif

	BYTE	MOUSE_SW;
	BYTE	JOYPAD1;
	BYTE	JOYPAD2;
	BYTE	JOY1BTN[4];

	COMCFG	mpu;
	COMCFG	com1;
	COMCFG	com2;
	COMCFG	com3;

	UINT32	clk_color1;
	UINT32	clk_color2;
	BYTE	clk_x;
	BYTE	clk_fnt;

	BYTE	comfirm;
	BYTE	shortcut;												// ver0.30

	BYTE	sstp;
	UINT16	sstpport;												// ver0.30

	BYTE	resume;													// ver0.30
	BYTE	statsave;
	BYTE	disablemmx;
	BYTE	wintype;
	BYTE	toolwin;
	BYTE	keydisp;
	BYTE	I286SAVE;
	BYTE	hostdrv_write;
	BYTE	jastsnd;
	BYTE	useromeo;
	BYTE	thickframe;
	BYTE	xrollkey;
#if defined(SUPPORT_OPRECORD)
	BYTE	oprecord;
#endif
} NP2OSCFG;


enum {
	FULLSCREEN_WIDTH	= 640,
	FULLSCREEN_HEIGHT	= 480
};

enum {
	NP2BREAK_MAIN		= 0x01,		// ウィンドウがinactiveになったのでエミュレーションを停止せよ
	NP2BREAK_DEBUG		= 0x02		// DebugUtilityがactiveになったのでエミュレーションを停止せよ
};

enum {
	IDM_TOOLWIN			= 20000,
	IDM_KEYDISP			= 20001,
	IDM_SOFTKBD			= 20002,
	IDM_MEMDBG32		= 20003,
	IDM_SCREENCENTER	= 20004,
	IDM_SNAPENABLE		= 20005,
	IDM_BACKGROUND		= 20006,
	IDM_BGSOUND			= 20007,
	IDM_TRACEONOFF		= 20008,
	IDM_MEMORYDUMP		= 20009,
	IDM_DEBUGUTY		= 20010,
	IDM_VIEWER			= 20011,
#if defined(VAEG_EXT)
	IDM_DEBUGCTRL		= 20030,
#endif

	IDM_SCRNMUL			= 20050,
	IDM_SCRNMUL4		= (IDM_SCRNMUL + 4),
	IDM_SCRNMUL6		= (IDM_SCRNMUL + 6),
	IDM_SCRNMUL8		= (IDM_SCRNMUL + 8),
	IDM_SCRNMUL10		= (IDM_SCRNMUL + 10),
	IDM_SCRNMUL12		= (IDM_SCRNMUL + 12),
	IDM_SCRNMUL16		= (IDM_SCRNMUL + 16),

	IDM_FLAGSAVE		= 20100,
	IDM_FLAGLOAD		= 20150,

#if defined(SUPPORT_OPRECORD)
	IDM_OPRECORDSTOP	= 20200,
	IDM_OPRECORDREPEAT	= 20201,
	IDM_OPRECORD_REC	= 20202,
	IDM_OPRECORD_PLAY	= 20203,
	IDM_OPRECORD_PLAYMULTI = 20204,

//	IDM_OPRECORDREC		= 20210,
//	IDM_OPRECORDPLAY	= 20250,
#endif

	WM_NP2CMD			= (WM_USER + 200),
	WM_SSTP				= (WM_USER + 201)
};

enum {
	NP2CMD_EXIT			= 0,
	NP2CMD_RESET		= 1,
	NP2CMD_EXIT2		= 0x0100,
	NP2CMD_DUMMY		= 0xffff
};

enum {
	MMXFLAG_DISABLE		= 1,
	MMXFLAG_NOTSUPPORT	= 2
};


extern	NP2OSCFG	np2oscfg;
extern	HWND		hWndMain;
extern	HINSTANCE	hInst;
extern	HINSTANCE	hPrev;
extern	int			mmxflag;
extern	BYTE		np2break;
extern	BOOL		winui_en;

extern	char		modulefile[MAX_PATH];
extern	char		fddfolder[MAX_PATH];
extern	char		hddfolder[MAX_PATH];
extern	char		bmpfilefolder[MAX_PATH];

void np2active_renewal(void);

