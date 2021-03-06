
enum {
	SKINMRU_MAX			= 4,
	FDDLIST_DRV			= 2,
	FDDLIST_MAX			= 8
};

typedef struct {
	int		insert;
	UINT	cnt;
	UINT	pos[FDDLIST_MAX];
	char	name[FDDLIST_MAX][MAX_PATH];
} TOOLFDD;

typedef struct {
	int		posx;
	int		posy;
	BOOL	type;
	TOOLFDD	fdd[FDDLIST_DRV];
	char	skin[MAX_PATH];
	char	skinmru[SKINMRU_MAX][MAX_PATH];
#if defined(SUPPORT_PC88VA)
	BYTE	modeled[3];
	BYTE	_dummy;
#endif
} NP2TOOL;


extern	NP2TOOL		np2tool;

BOOL toolwin_initapp(HINSTANCE hInstance);
void toolwin_create(void);
void toolwin_destroy(void);
HWND toolwin_gethwnd(void);

void toolwin_setfdd(BYTE drv, const char *name);

#ifdef __cplusplus
extern "C" {
#endif

#if defined(VAEG_EXT)
void toolwin_fddaccess(BYTE drv, int mediatype);
#else
void toolwin_fddaccess(BYTE drv);
#endif
void toolwin_hddaccess(BYTE drv);

#if defined(SUPPORT_PC88VA)
void toolwin_modeled(BYTE num, BYTE sw);
#endif

#ifdef __cplusplus
}
#endif
void toolwin_draw(BYTE frame);

void toolwin_readini(void);
void toolwin_writeini(void);

