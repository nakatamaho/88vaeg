#include	"compiler.h"
#include	"commctrl.h"		// VAEG_EXT _TrackMouseEvent
#include	"strres.h"
#include	"resource.h"
#include	"np2.h"
#include	"winloc.h"
#include	"dosio.h"
#include	"soundmng.h"
#include	"sysmng.h"
#include	"menu.h"
#include	"toolwin.h"
#include	"ini.h"
#include	"np2class.h"
#include	"dialog.h"
#include	"dialogs.h"
#include	"pccore.h"
#include	"diskdrv.h"

#if defined(VAEG_EXT)
#if _WIN32_WINNT >= 0x0500
#define SUPPORT_XPARENT			// 透明色設定を有効にする
#endif
#endif

extern WINLOCEX np2_winlocexallwin(HWND base);


enum {
	IDC_TOOLHDDACC			= 0,
	IDC_TOOLFDD1ACC,
	IDC_TOOLFDD1LIST,
	IDC_TOOLFDD1BROWSE,
	IDC_TOOLFDD1EJECT,
	IDC_TOOLFDD2ACC,
	IDC_TOOLFDD2LIST,
	IDC_TOOLFDD2BROWSE,
	IDC_TOOLFDD2EJECT,
	IDC_TOOLRESET,
	IDC_TOOLPOWER,
#if defined(SUPPORT_PC88VA)
	IDC_TOOLV1MODE,
	IDC_TOOLV2MODE,
	IDC_TOOLV3MODE,
#endif
#if defined(VAEG_EXT)
	IDC_TOOLFDD1DOOR,
	IDC_TOOLFDD2DOOR,
#endif
	IDC_MAXITEMS,

	IDC_BASE				= 3000,

	IDM_SKINSEL				= 3100,
	IDM_SKINDEF				= 3101,
	IDM_SKINMRU				= 3102,
	IDM_TOOLCLOSE			= IDM_SKINMRU + SKINMRU_MAX
};

enum {
	TCTL_STATIC				= 0,
	TCTL_BUTTON				= 1,
	TCTL_DDLIST				= 2
};

typedef struct {
	char	main[MAX_PATH];
	char	font[64];
	SINT32	fontsize;
	UINT32	color1;
	UINT32	color2;
#if defined(VAEG_EXT)
	UINT32	transparentcolor;
#endif
} TOOLSKIN;

typedef struct {
	UINT	tctl;
const char	*text;
	short	posx;
	short	posy;
	short	width;
	short	height;
	short	extend;
	short	padding;
} SUBITEM;

typedef struct {
	HWND			hwnd;
	WINLOCEX		wlex;
	HBITMAP			hbmp;
#if defined(SUPPORT_PC88VA)
	HBITMAP			hbmp2;
#endif
	BYTE			fddaccess[2];
	BYTE			hddaccess;
#if defined(VAEG_EXT)
	BYTE			fddaccesscolor[2];
	BYTE			hddaccesscolor;
#endif
/*
#if defined(SUPPORT_PC88VA)
	BYTE			modeled[3];
#endif
*/
	BYTE			_padding;
	int				winflg;
	int				wingx;
	int				wingy;
	int				wintx;
	int				winty;
	UINT			parentcn;
	int				parentx;
	int				parenty;
	HFONT			hfont;
	HDC				hdcfont;
#if defined(SUPPORT_PC88VA) || defined(VAEG_EXT)
	HBRUSH			access[4];	//0..消灯(赤) 1..赤  3..消灯(緑) 4..緑
#else
	HBRUSH			access[2];
#endif
	HWND			sub[IDC_MAXITEMS];
	SUBCLASSPROC	subproc[IDC_MAXITEMS];
#if defined(VAEG_EXT)
	int				subext[IDC_MAXITEMS];
	BOOL			mouseover;
	BOOL			mousetracking;
	BOOL			useroperating;
	int				quietdelaycount;
#endif
} TOOLWIN;

enum {
	GTWL_FOCUS		= NP2GWL_SIZE + 0,
	GTWL_SIZE		= NP2GWL_SIZE + 4
};

#if defined(VAEG_EXT)
enum {	// TOOLWIN.subext
	SUBEXT_QUIET	= 1,
};
#endif




		NP2TOOL		np2tool;
static	TOOLSKIN	toolskin;
static	SUBITEM		subitem[IDC_MAXITEMS];
static	TOOLWIN		toolwin;

#include	"toolwin.res"


typedef struct {
	WORD	idc;
	BYTE	*counter;
#if defined(VAEG_EXT)
	BYTE	*color;		// 0..赤 2..緑
#endif
} DISKACC;

static const BYTE fddlist[FDDLIST_DRV] = {
					IDC_TOOLFDD1LIST, IDC_TOOLFDD2LIST};

#if defined(VAEG_EXT)
static const DISKACC diskacc[3] = {
			{IDC_TOOLFDD1ACC,	&toolwin.fddaccess[0], &toolwin.fddaccesscolor[0]},
			{IDC_TOOLFDD2ACC,	&toolwin.fddaccess[1], &toolwin.fddaccesscolor[1]},
			{IDC_TOOLHDDACC,	&toolwin.hddaccess,	   &toolwin.hddaccesscolor}};
#else
static const DISKACC diskacc[3] = {
					{IDC_TOOLFDD1ACC,	&toolwin.fddaccess[0]},
					{IDC_TOOLFDD2ACC,	&toolwin.fddaccess[1]},
					{IDC_TOOLHDDACC,	&toolwin.hddaccess}};
#endif

// ----

static HBITMAP skinload(const char *path) {

	char	fname[MAX_PATH];
	UINT	i;
	HBITMAP	ret;

	ZeroMemory(&toolskin, sizeof(toolskin));
	toolskin.fontsize = 12;
	milstr_ncpy(toolskin.font, str_deffont, sizeof(toolskin.font));
	toolskin.color1 = 0x600000;
	toolskin.color2 = 0xff0000;
#if defined(VAEG_EXT)
	toolskin.transparentcolor = 0xffffffff;
#endif
	if (path) {
		ini_read(path, skintitle, skinini1, sizeof(skinini1)/sizeof(INITBL));
	}
	if (toolskin.main[0]) {
		ZeroMemory(subitem, sizeof(defsubitem));
		for (i=0; i<IDC_MAXITEMS; i++) {
			subitem[i].tctl = defsubitem[i].tctl;
			subitem[i].text = defsubitem[i].text;
		}
	}
	else {
		CopyMemory(subitem, defsubitem, sizeof(defsubitem));
	}
	if (path) {
		ini_read(path, skintitle, skinini2, sizeof(skinini2)/sizeof(INITBL));
	}
	if (toolskin.main[0]) {
		milstr_ncpy(fname, path, sizeof(fname));
		file_cutname(fname);
		file_catname(fname, toolskin.main, sizeof(fname));
		ret = (HBITMAP)LoadImage(hInst, fname, IMAGE_BITMAP,
													0, 0, LR_LOADFROMFILE);
		if (ret != NULL) {
			return(ret);
		}
	}
#if defined(VAEG_EXT)
	{
		HBITMAP hbmp;

		hbmp = LoadBitmap(hInst, "NP2TOOL");
		if (hbmp != NULL) {
			toolwin.hbmp2 = LoadBitmap(hInst, "NP2TOOL2");
		}
		return(hbmp);
	}
#else
	return(LoadBitmap(hInst, "NP2TOOL"));
#endif
}


// ----

static void calctextsize(char *path, int leng, const char *p, int width) {

	HDC		hdc;
	SIZE	cur;
	char	work[MAX_PATH];
	int		l;
	SIZE	tail;
	int		pos;
	int		step;

	milstr_ncpy(path, p, leng);
	hdc = toolwin.hdcfont;
	GetTextExtentPoint32(hdc, p, strlen(p), &cur);
	if (cur.cx < width) {
		return;
	}

	file_cutname(path);
	file_cutseparator(path);
	file_cutname(path);
	file_cutseparator(path);
	l = strlen(path);
	work[0] = '\0';
	if (l) {
		milstr_ncpy(work, str_browse, sizeof(work));
	}
	milstr_ncat(work, p + l, sizeof(work));
	GetTextExtentPoint32(hdc, work, strlen(work), &tail);
	pos = 0;
	while(pos < l) {
		step = ((((p[pos] ^ 0x20) - 0xa1) & 0xff) < 0x3c)?2:1;
		GetTextExtentPoint32(hdc, p, pos + step, &cur);
		if (cur.cx + tail.cx >= width) {
			break;
		}
		pos += step;
	}
	if (pos < leng) {
		path[pos] = '\0';
	}
	milstr_ncat(path, work, leng);
}

static void setlist(HWND hwnd, const TOOLFDD *fdd, UINT sel) {

	RECT	rc;
	int		width;
	char	basedir[MAX_PATH];
	UINT	i;
const char	*p;
	char	dir[MAX_PATH];
const char	*q;

	SendMessage(hwnd, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
	GetClientRect(hwnd, &rc);
	width = rc.right - rc.left - 6;			// border size?
	basedir[0] = '\0';
	if (sel < fdd->cnt) {
		milstr_ncpy(basedir, fdd->name[fdd->pos[sel]], sizeof(basedir));
		file_cutname(basedir);
	}
	for (i=0; i<fdd->cnt; i++) {
		p = fdd->name[fdd->pos[i]];
		milstr_ncpy(dir, p, sizeof(dir));
		file_cutname(dir);
		if (!file_cmpname(basedir, dir)) {
			q = file_getname((char *)p);
		}
		else {
			calctextsize(dir, sizeof(dir), p, width);
			q = dir;
		}
		SendMessage(hwnd, CB_INSERTSTRING, (WPARAM)i, (LPARAM)q);
		p += sizeof(fdd->name[0]);
	}
	if (sel < fdd->cnt) {
		SendMessage(hwnd, CB_SETCURSEL, (WPARAM)sel, (LPARAM)0);
	}
}

static void sellist(UINT drv) {

	HWND	hwnd;
	TOOLFDD	*fdd;
	UINT	sel;

	if (drv >= FDDLIST_DRV) {
		return;
	}
	hwnd = toolwin.sub[fddlist[drv]];
	fdd = np2tool.fdd + drv;
	sel = (UINT)SendMessage(hwnd, CB_GETCURSEL, 0, 0);
	if (sel < fdd->cnt) {
		diskdrv_setfdd(drv, fdd->name[fdd->pos[sel]], 0);
		fdd->insert = 1;
		setlist(hwnd, fdd, sel);
#if defined(VAEG_EXT)
		if (toolwin.hwnd) {
			InvalidateRect(toolwin.hwnd, NULL, TRUE);
		}
#endif
	}
}

static void remakefddlist(HWND hwnd, TOOLFDD *fdd) {

	char	*p;
	UINT	cnt;
	char	*q;
	char	*fname[FDDLIST_MAX];
	UINT	i;
	UINT	j;
	UINT	sel;

	p = fdd->name[0];
	for (cnt=0; cnt<FDDLIST_MAX; cnt++) {
		if (p[0] == '\0') {
			break;
		}
		q = file_getname(p);
		fname[cnt] = q;
		for (i=0; i<cnt; i++) {
			if (file_cmpname(q, fname[fdd->pos[i]]) < 0) {
				break;
			}
		}
		for (j=cnt; j>i; j--) {
			fdd->pos[j] = fdd->pos[j-1];
		}
		fdd->pos[i] = cnt;
		p += sizeof(fdd->name[0]);
	}
	fdd->cnt = cnt;
	sel = (UINT)-1;
	if (fdd->insert) {
		for (i=0; i<cnt; i++) {
			if (fdd->pos[i] == 0) {
				sel = i;
				break;
			}
		}
	}
	setlist(hwnd, fdd, sel);
}

#if defined(VAEG_EXT)
static void accdraw(HWND hWnd, BYTE count, int color) {
#else
static void accdraw(HWND hWnd, BYTE count) {
#endif
	HDC			hdc;
	PAINTSTRUCT	ps;
	RECT		rc;
	HBRUSH		hbrush;

	hdc = BeginPaint(hWnd, &ps);
	GetClientRect(hWnd, &rc);
#if defined(VAEG_EXT)
	hbrush = (HBRUSH)SelectObject(hdc, toolwin.access[(count)? color + 1: color]);
#else
	hbrush = (HBRUSH)SelectObject(hdc, toolwin.access[(count)?1:0]);
#endif
	PatBlt(hdc, rc.left, rc.top, rc.right, rc.bottom, PATCOPY);
	SelectObject(hdc, hbrush);
	EndPaint(hWnd, &ps);
}

#if defined(SUPPORT_PC88VA)
static void modedraw(HWND hWnd, BYTE sw) {

	HDC			hdc;
	PAINTSTRUCT	ps;
	RECT		rc;
	HBRUSH		hbrush;

	hdc = BeginPaint(hWnd, &ps);
	GetClientRect(hWnd, &rc);
	hbrush = (HBRUSH)SelectObject(hdc, toolwin.access[(sw)?3:2]);
	PatBlt(hdc, rc.left, rc.top, rc.right, rc.bottom, PATCOPY);
	SelectObject(hdc, hbrush);
	EndPaint(hWnd, &ps);
}
#endif

#if defined(VAEG_EXT)
static void toolwinpaint2(HWND hWnd, HDC hdc);

static void doordraw(HWND hWnd, int doorstat) {
	HDC			hdc;
	PAINTSTRUCT	ps;

	hdc = BeginPaint(hWnd, &ps);
	toolwinpaint2(hWnd, hdc);
	EndPaint(hWnd, &ps);
}
#endif


// ---- コントロールのWindowProcedure

static LRESULT CALLBACK twsub(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {

	UINT	idc;
	int		dir;
	UINT	newidc;
	int		files;
	char	fname[MAX_PATH];

	idc = GetWindowLong(hWnd, GWL_ID) - IDC_BASE;
	if (idc >= IDC_MAXITEMS) {
		return(0);
	}
	if (msg == WM_KEYDOWN) {
		if ((short)wp == VK_TAB) {
			dir = (GetKeyState(VK_SHIFT) >= 0)?1:-1;
			newidc = idc;
			do {
				newidc += dir;
				if (newidc >= IDC_MAXITEMS) {
					newidc = (dir >= 0)?0:(IDC_MAXITEMS - 1);
				}
				if ((toolwin.sub[newidc] != NULL) &&
					(subitem[newidc].tctl != TCTL_STATIC)) {
					SetFocus(toolwin.sub[newidc]);
					break;
				}
			} while(idc != newidc);
		}
		else if ((short)wp == VK_RETURN) {
			if (subitem[idc].tctl == TCTL_BUTTON) {
				return(CallWindowProc(toolwin.subproc[idc],
										hWnd, WM_KEYDOWN, VK_SPACE, 0));
			}
		}
	}
	else if (msg == WM_DROPFILES) {
   	    files = DragQueryFile((HDROP)wp, (UINT)-1, NULL, 0);
		if (files == 1) {
			DragQueryFile((HDROP)wp, 0, fname, sizeof(fname));
			if (idc == IDC_TOOLFDD1LIST) {
				diskdrv_setfdd(0, fname, 0);
				toolwin_setfdd(0, fname);
			}
			else if (idc == IDC_TOOLFDD2LIST) {
				diskdrv_setfdd(1, fname, 0);
				toolwin_setfdd(1, fname);
			}
		}
		DragFinish((HDROP)wp);
		return(FALSE);
	}
	else if (msg == WM_PAINT) {
		if (idc == IDC_TOOLHDDACC) {
#if defined(VAEG_EXT)
			accdraw(hWnd, toolwin.hddaccess, 0);
#else
			accdraw(hWnd, toolwin.hddaccess);
#endif
			return(FALSE);
		}
		else if (idc == IDC_TOOLFDD1ACC) {
#if defined(VAEG_EXT)
			accdraw(hWnd, toolwin.fddaccess[0], toolwin.fddaccesscolor[0]);
#else
			accdraw(hWnd, toolwin.fddaccess[0]);
#endif
			return(FALSE);
		}
		else if (idc == IDC_TOOLFDD2ACC) {
#if defined(VAEG_EXT)
			accdraw(hWnd, toolwin.fddaccess[1], toolwin.fddaccesscolor[1]);
#else
			accdraw(hWnd, toolwin.fddaccess[1]);
#endif
			return(FALSE);
		}
#if defined(SUPPORT_PC88VA)
		else if (idc == IDC_TOOLV1MODE) {
			modedraw(hWnd, np2tool.modeled[0]);
			return(FALSE);
		}
		else if (idc == IDC_TOOLV2MODE) {
			modedraw(hWnd, np2tool.modeled[1]);
			return(FALSE);
		}
		else if (idc == IDC_TOOLV3MODE) {
			modedraw(hWnd, np2tool.modeled[2]);
			return(FALSE);
		}
#endif
#if defined(VAEG_EXT)
		else if (idc == IDC_TOOLFDD1DOOR) {
			doordraw(hWnd, np2tool.fdd[0].insert);
			return(FALSE);
		}
		else if (idc == IDC_TOOLFDD2DOOR) {
			doordraw(hWnd, np2tool.fdd[1].insert);
			return(FALSE);
		}
#endif
	}
	else if (msg == WM_SETFOCUS) {
		SetWindowLong(GetParent(hWnd), GTWL_FOCUS, idc);
	}
	return(CallWindowProc(toolwin.subproc[idc], hWnd, msg, wp, lp));
}


// ---- メインウィンドウのWindowProcedureから使うサブルーチン

static void toolwincreate(HWND hWnd) {

	HDC			hdc;
const SUBITEM	*p;
	UINT		i;
	HWND		sub;
const char		*cls;
	DWORD		style;
#if defined(VAEG_EXT)
	int			subext;
#endif

#if defined(VAEG_EXT)
#if defined(SUPPORT_XPARENT)
	if (toolskin.transparentcolor <= 0xffffff) {
		COLORREF color;

		color = RGB((toolskin.transparentcolor >> 16) & 0xff,
					(toolskin.transparentcolor >>  8) & 0xff,
					toolskin.transparentcolor & 0xff);
		SetWindowLong(hWnd, GWL_EXSTYLE,
				GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
		SetLayeredWindowAttributes(hWnd, color, 255, LWA_COLORKEY);
	}
	else {
		SetWindowLong(hWnd, GWL_EXSTYLE,
				GetWindowLong(hWnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
	}
#endif
#endif

	toolwin.hfont = CreateFont(toolskin.fontsize, 0, 0, 0, 0, 0, 0, 0,
					SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
					DEFAULT_QUALITY, FIXED_PITCH, toolskin.font);
    hdc = GetDC(NULL);
	toolwin.hdcfont = CreateCompatibleDC(hdc);
	ReleaseDC(NULL, hdc);
	SelectObject(toolwin.hdcfont, toolwin.hfont);

#if defined(SUPPORT_PC88VA)
	toolwin.access[0] = CreateSolidBrush(0x405040);
	toolwin.access[1] = CreateSolidBrush(0x4080ff);
#else
	toolwin.access[0] = CreateSolidBrush(0x000060);
	toolwin.access[1] = CreateSolidBrush(0x0000ff);
#endif
#if defined(SUPPORT_PC88VA) || defined(VAEG_EXT)
	toolwin.access[2] = CreateSolidBrush(0x405040);
	toolwin.access[3] = CreateSolidBrush(0x00ff00);
#endif

	p = subitem;
	for (i=0; i<IDC_MAXITEMS; i++) {
		sub = NULL;
		cls = NULL;
#if defined(VAEG_EXT)
		subext = 0;
#endif
		switch(p->tctl) {
			case TCTL_STATIC:
				cls = str_static;
				style = 0;
				break;

			case TCTL_BUTTON:
				if (p->extend == 0) {
					cls = str_button;
					style = BS_PUSHBUTTON;
				}
				else if (p->extend == 1) {
					cls = str_button;
					style = BS_OWNERDRAW;
				}
#if defined(VAEG_EXT)
				else if (p->extend == 2) {
					cls = str_button;
					style = BS_PUSHBUTTON;
					subext = SUBEXT_QUIET;
				}
#endif
				break;

			case TCTL_DDLIST:
				cls = str_combobox;
				style = CBS_DROPDOWNLIST | WS_VSCROLL;
#if defined(VAEG_EXT)
				if (p->extend == 2) {
					subext = SUBEXT_QUIET;
				}
#endif
				break;
		}
		if ((cls) && (p->width > 0) && (p->height > 0)) {
#if defined(VAEG_EXT)
			if (!(subext & SUBEXT_QUIET)) style |= WS_VISIBLE;
			sub = CreateWindow(cls, p->text, WS_CHILD | WS_CLIPSIBLINGS | style,
							p->posx, p->posy, p->width, p->height,
							hWnd, (HMENU)(i + IDC_BASE), hInst, NULL);
#else
			sub = CreateWindow(cls, p->text, WS_CHILD | WS_VISIBLE | style,
							p->posx, p->posy, p->width, p->height,
							hWnd, (HMENU)(i + IDC_BASE), hInst, NULL);
#endif
		}
		toolwin.sub[i] = sub;
		toolwin.subproc[i] = NULL;
#if defined(VAEG_EXT)
		toolwin.subext[i] = subext;
#endif
		if (sub) {
			toolwin.subproc[i] = (SUBCLASSPROC)GetWindowLong(sub, GWL_WNDPROC);
			SetWindowLong(sub, GWL_WNDPROC, (LONG)twsub);
			SendMessage(sub, WM_SETFONT, (WPARAM)toolwin.hfont,
														MAKELPARAM(TRUE, 0));
		}
		p++;
	}
	for (i=0; i<FDDLIST_DRV; i++) {
		sub = toolwin.sub[fddlist[i]];
		if (sub) {
			DragAcceptFiles(sub, TRUE);
			remakefddlist(sub, np2tool.fdd + i);
		}
	}
	for (i=0; i<IDC_MAXITEMS; i++) {
		if ((toolwin.sub[i]) && (subitem[i].tctl != TCTL_STATIC)) {
			break;
		}
	}
	SetWindowLong(hWnd, GTWL_FOCUS, i);
}

static void toolwindestroy(void) {

	UINT	i;
	HWND	sub;

	if (toolwin.hbmp) {
		for (i=0; i<IDC_MAXITEMS; i++) {
			sub = toolwin.sub[i];
			if (sub) {
				DestroyWindow(sub);
			}
		}
		DeleteObject(toolwin.access[0]);
		DeleteObject(toolwin.access[1]);
#if defined(SUPPORT_PC88VA) || defined(VAEG_EXT)
		DeleteObject(toolwin.access[2]);
		DeleteObject(toolwin.access[3]);
#endif
		DeleteObject(toolwin.hdcfont);
		DeleteObject(toolwin.hfont);
		DeleteObject(toolwin.hbmp);
#if defined(VAEG_EXT)
		if (toolwin.hbmp2) {
			DeleteObject(toolwin.hbmp2);
			toolwin.hbmp2 = NULL;
		}
#endif
		toolwin.hbmp = NULL;
	}
}


#if defined(VAEG_EXT)

/*
hbmpのrectの部分をhWndに描画する。
hbmpはtool windowの左上隅に位置するとする。
*/
static void paintskin(HWND hWnd, HDC hdc, HBITMAP *hbmp, RECT *rect)
{
	RECT dstrect;
	HDC hmdc;
	POINT oldorg;

	hmdc = CreateCompatibleDC(hdc);

	GetClientRect(hWnd, &dstrect);	// dstrectはhWndの座標系

	MapWindowPoints(hWnd, toolwin.hwnd, (POINT *)&dstrect ,2);

	// tool windowの座標系にする
	SetViewportOrgEx(hdc, -dstrect.left, -dstrect.top, &oldorg);
	SelectObject(hmdc, *hbmp);
	// ただ単純に、hbmp, rectで指定された画像、部分を描画
	BitBlt(hdc, rect->left, rect->top, rect->right, rect->bottom,
			   hmdc, rect->left, rect->top, SRCCOPY);
	//元の座標系にもどす
	SetViewportOrgEx(hdc, oldorg.x, oldorg.y, NULL);
	DeleteDC(hmdc);
}


// ドライブドアの状況を含めて描画
static void toolwinpaint2(HWND hWnd, HDC hdc) {
	RECT		rc;
	RECT		doorRc;
	HWND		doorhWnd;
	BITMAP		bmp;
	int			i;

	if (toolwin.hbmp) {
		// スキン(通常状態)
		GetObject(toolwin.hbmp, sizeof(BITMAP), &bmp);
		rc.top = 0;
		rc.left = 0;
		rc.right = bmp.bmWidth;
		rc.bottom = bmp.bmHeight;
		paintskin(hWnd, hdc, &toolwin.hbmp, &rc); 

		// スキン(ドアクローズ状態)
		if (toolwin.hbmp2) {
			for (i = 0; i <FDDLIST_DRV; i++) {
				if (np2tool.fdd[i].insert) {
					doorhWnd = toolwin.sub[IDC_TOOLFDD1DOOR + i];
					if (doorhWnd) {
						GetClientRect(doorhWnd, &doorRc);
						MapWindowPoints(doorhWnd, toolwin.hwnd, (POINT *)&doorRc, 2); 
						paintskin(hWnd, hdc, &toolwin.hbmp2, &doorRc);
					}
				}
			}
		}

	}

}

static void toolwinpaint(HWND hWnd) {
	HDC hdc;
	PAINTSTRUCT	ps;

	hdc = BeginPaint(hWnd, &ps);
	toolwinpaint2(hWnd, hdc);
	EndPaint(hWnd, &ps);
}

#else

static void toolwinpaint(HWND hWnd) {

	HDC			hdc;
	PAINTSTRUCT	ps;
	BITMAP		bmp;
	HDC			hmdc;

	hdc = BeginPaint(hWnd, &ps);
	if (toolwin.hbmp) {
		GetObject(toolwin.hbmp, sizeof(BITMAP), &bmp);
		hmdc = CreateCompatibleDC(hdc);
		SelectObject(hmdc, toolwin.hbmp);
		BitBlt(hdc, 0, 0, bmp.bmWidth, bmp.bmHeight, hmdc, 0, 0, SRCCOPY);
		DeleteDC(hmdc);
	}
	EndPaint(hWnd, &ps);
}
#endif

#if defined(VAEG_EXT)
static void tooldrawbutton(HWND hWnd, UINT idc, LPDRAWITEMSTRUCT lpdis) {

	HWND	sub;
	HDC		hdc;
	RECT	btn;

	idc -= IDC_BASE;
	if (idc >= IDC_MAXITEMS) {
		return;
	}
	hdc = lpdis->hDC;
	sub = toolwin.sub[idc];
	toolwinpaint2(sub, hdc);

	// フォーカスを持っている場合に表示する枠
	if (lpdis->itemState & ODS_FOCUS) {
		GetClientRect(sub, &btn);
		DrawFocusRect(hdc, &btn);
	}
}
#else

static void tooldrawbutton(HWND hWnd, UINT idc, LPDRAWITEMSTRUCT lpdis) {

	POINT	pt;
	HWND	sub;
	RECT	rect;
	HDC		hdc;
	HDC		hmdc;
	RECT	btn;

	idc -= IDC_BASE;
	if (idc >= IDC_MAXITEMS) {
		return;
	}
	pt.x = 0;
	pt.y = 0;
	ClientToScreen(hWnd, &pt);
	sub = toolwin.sub[idc];
	GetWindowRect(sub, &rect);
	btn.left = 0;
	btn.top = 0;
	btn.right = rect.right - rect.left;
	btn.bottom = rect.bottom - rect.top;
	hdc = lpdis->hDC;
	if (toolwin.hbmp) {
		hmdc = CreateCompatibleDC(hdc);
		SelectObject(hmdc, toolwin.hbmp);
		BitBlt(hdc, 0, 0, btn.right, btn.bottom,
					hmdc, rect.left - pt.x, rect.top - pt.y, SRCCOPY);
		DeleteDC(hmdc);
	}
	if (lpdis->itemState & ODS_FOCUS) {
		DrawFocusRect(hdc, &btn);
	}
}
#endif

// ---- スキンメニュー

static HMENU createskinmenu(void) {

	HMENU	ret;
	UINT	cnt;
const char	*base;
	UINT	flag;
	char	*p;
	UINT	i;
	UINT	j;
	UINT	id[SKINMRU_MAX];
const char	*file[SKINMRU_MAX];

	ret = CreatePopupMenu();
	AppendMenu(ret, MF_STRING, IDM_SKINSEL, str_skinsel);
	AppendMenu(ret, MF_SEPARATOR, 0, NULL);

	base = np2tool.skin;
	flag = (base[0] == '\0')?MF_CHECKED:0;
	AppendMenu(ret, MF_STRING + flag, IDM_SKINDEF, str_skindef);
	for (cnt=0; cnt<SKINMRU_MAX; cnt++) {
		p = np2tool.skinmru[cnt];
		if (p[0] == '\0') {
			break;
		}
		p = file_getname(p);
		for (i=0; i<cnt; i++) {
			if (file_cmpname(p, file[id[i]]) < 0) {
				break;
			}
		}
		for (j=cnt; j>i; j--) {
			id[j] = id[j-1];
		}
		id[i] = cnt;
		file[cnt] = p;
	}
	for (i=0; i<cnt; i++) {
		j = id[i];
		flag = (!file_cmpname(base, np2tool.skinmru[j]))?MF_CHECKED:0;
		AppendMenu(ret, MF_STRING + flag, IDM_SKINMRU + j, file[j]);
	}
	return(ret);
}

static void skinchange(HWND hWnd) {

const char		*p;
	UINT		i;
	HBITMAP		hbmp;
	BITMAP		bmp;
	WINLOCEX	wlex;

	p = np2tool.skin;
	if (p[0]) {
		for (i=0; i<(SKINMRU_MAX - 1); i++) {
			if (!file_cmpname(p, np2tool.skinmru[i])) {
				break;
			}
		}
		while(i > 0) {
			CopyMemory(np2tool.skinmru[i], np2tool.skinmru[i-1],
												sizeof(np2tool.skinmru[0]));
			i--;
		}
		file_cpyname(np2tool.skinmru[0], p, sizeof(np2tool.skinmru[0]));
	}
	ModifyMenu(np2class_gethmenu(hWnd), 0, MF_BYPOSITION | MF_POPUP,
									(UINT)createskinmenu(), str_toolskin);
	ModifyMenu(GetSystemMenu(hWnd, FALSE), 0, MF_BYPOSITION | MF_POPUP,
									(UINT)createskinmenu(), str_toolskin);
	DrawMenuBar(hWnd);
	sysmng_update(SYS_UPDATEOSCFG);

	wlex = np2_winlocexallwin(hWndMain);
	winlocex_setholdwnd(wlex, hWnd);
	toolwindestroy();
	hbmp = skinload(np2tool.skin);
	if (hbmp == NULL) {
		SendMessage(hWnd, WM_CLOSE, 0, 0);
		return;
	}
	GetObject(hbmp, sizeof(BITMAP), &bmp);
	toolwin.hbmp = hbmp;
	winloc_setclientsize(hWnd, bmp.bmWidth, bmp.bmHeight);
	toolwincreate(hWnd);
	winlocex_move(wlex);
	winlocex_destroy(wlex);
}


// ---- マウスカーソルがツールウィンドウ上にあるときだけコントロールを表示する

#if defined(VAEG_EXT)

/*
タイマーを始動
*/
static void startmousetracking(void) {
	if (!toolwin.mousetracking) {
		SetTimer(toolwin.hwnd, 1, 500, NULL);
		toolwin.mousetracking = TRUE;
	}
}

/*
コントロールを表示
*/
static void prominent(void) {
	int i;

	for (i=0; i<IDC_MAXITEMS; i++) {
		if (toolwin.subext[i] & SUBEXT_QUIET) {
			if (toolwin.sub[i]) {
				ShowWindow(toolwin.sub[i],SW_SHOW);
			}
		}
	}
	toolwin.quietdelaycount = 0;
}

/*
コントロールを非表示
*/
static void quiet(void) {
	int i;

	for (i=0; i<IDC_MAXITEMS; i++) {
		if (toolwin.subext[i] & SUBEXT_QUIET) {
			if (toolwin.sub[i]) {
				ShowWindow(toolwin.sub[i],SW_HIDE);
			}
		}
	}
	toolwin.quietdelaycount = 0;
}

/*
少したったらコントロールを非表示
*/
static void quietsoon(void) {
	toolwin.quietdelaycount = 6;
	startmousetracking();
}

/*
少したったかどうか確認
*/
static void checkquietdelaycount(void) {
	if (toolwin.quietdelaycount) {
		toolwin.quietdelaycount--;
		if (toolwin.quietdelaycount == 0) {
			quiet();
		}
	}
}

/*
マウスカーソルがツールウィンドウ内に入ったかどうかを確認
*/
static void checkmouseenter(void) {
	if (!toolwin.mouseover) {
		RECT rect;
		POINT pt;

		GetClientRect(toolwin.hwnd,&rect);
		MapWindowPoints(toolwin.hwnd,NULL,(LPPOINT)&rect,2);
		GetCursorPos(&pt);
		if (PtInRect(&rect,pt)) {
			prominent();
			toolwin.mouseover = TRUE;
		}
	}
}

/*
マウスカーソルがツールウィンドウ外に出たかどうかを確認
*/
static void checkmouseleave(void) {
	if (toolwin.mouseover && !toolwin.useroperating) {
		RECT rect;
		POINT pt;

		GetClientRect(toolwin.hwnd,&rect);
		MapWindowPoints(toolwin.hwnd,NULL,(LPPOINT)&rect,2);
		GetCursorPos(&pt);
		if (!PtInRect(&rect,pt)) {
			quietsoon();
			toolwin.mouseover = FALSE;
		}
	}
}

#endif


// ---- ポップアップメニュー

static void openpopup(HWND hWnd, LPARAM lp) {

	HMENU	hMenu;
	HMENU	mainmenu;
	POINT	pt;

	hMenu = CreatePopupMenu();
	if (!winui_en) {
		mainmenu = np2class_gethmenu(hWndMain);
		menu_addmenubar(hMenu, mainmenu);
	}
	AppendMenu(hMenu, MF_POPUP, (UINT)createskinmenu(), str_toolskin);
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, IDM_TOOLCLOSE, str_toolclose);
	pt.x = LOWORD(lp);
	pt.y = HIWORD(lp);
	ClientToScreen(hWnd, &pt);
	TrackPopupMenu(hMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
	DestroyMenu(hMenu);
}


// ---- ウィンドウのWindowProcedure

static LRESULT CALLBACK twproc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {

	HMENU		hMenu;
	BOOL		r;
	UINT		idc;
	WINLOCEX	wlex;

	switch(msg) {
		case WM_CREATE:
			np2class_wmcreate(hWnd);
			ModifyMenu(np2class_gethmenu(hWnd), 0, MF_BYPOSITION | MF_POPUP,
									(UINT)createskinmenu(), str_toolskin);
			hMenu = GetSystemMenu(hWnd, FALSE);
			InsertMenu(hMenu, 0, MF_BYPOSITION | MF_POPUP,
									(UINT)createskinmenu(), str_toolskin);
			InsertMenu(hMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

			np2class_windowtype(hWnd, (np2tool.type & 1) << 1);
			toolwincreate(hWnd);
			break;

		case WM_SYSCOMMAND:
			switch(wp) {
				case IDM_SKINSEL:
				case IDM_SKINDEF:
				case IDM_SKINMRU + 0:
				case IDM_SKINMRU + 1:
				case IDM_SKINMRU + 2:
				case IDM_SKINMRU + 3:
					return(SendMessage(hWnd, WM_COMMAND, wp, lp));

				default:
					return(DefWindowProc(hWnd, msg, wp, lp));
			}
			break;

		case WM_COMMAND:
			switch(LOWORD(wp)) {
				case IDC_BASE + IDC_TOOLFDD1LIST:
					if (HIWORD(wp) == CBN_SELCHANGE) {
						sellist(0);
					}
#if defined(VAEG_EXT)
					else if (HIWORD(wp) == CBN_DROPDOWN) {
						toolwin.useroperating = TRUE;
					}
					else if (HIWORD(wp) == CBN_CLOSEUP) {
						toolwin.useroperating = FALSE;
						//checkmouseleave();
					}
#endif
					break;

				case IDC_BASE + IDC_TOOLFDD1BROWSE:
					if (!winui_en) {
						SendMessage(hWndMain, WM_COMMAND, IDM_FDD1OPEN, 0);
					}
					break;

				case IDC_BASE + IDC_TOOLFDD1EJECT:
					diskdrv_setfdd(0, NULL, 0);
					toolwin_setfdd(0, NULL);
					break;

				case IDC_BASE + IDC_TOOLFDD2LIST:
					if (HIWORD(wp) == CBN_SELCHANGE) {
						sellist(1);
					}
#if defined(VAEG_EXT)
					else if (HIWORD(wp) == CBN_DROPDOWN) {
						toolwin.useroperating = TRUE;
					}
					else if (HIWORD(wp) == CBN_CLOSEUP) {
						toolwin.useroperating = FALSE;
						//checkmouseleave();
					}
#endif
					break;

				case IDC_BASE + IDC_TOOLFDD2BROWSE:
					if (!winui_en) {
						SendMessage(hWndMain, WM_COMMAND, IDM_FDD2OPEN, 0);
					}
					break;

				case IDC_BASE + IDC_TOOLFDD2EJECT:
					diskdrv_setfdd(1, NULL, 0);
					toolwin_setfdd(1, NULL);
					break;

				case IDC_BASE + IDC_TOOLRESET:
					if (!winui_en) {
						SendMessage(hWndMain, WM_COMMAND, IDM_RESET, 0);
						SetForegroundWindow(hWndMain);
					}
					break;

				case IDC_BASE + IDC_TOOLPOWER:
					if (!winui_en) {
						SendMessage(hWndMain, WM_CLOSE, 0, 0L);
					}
					break;

				case IDM_SKINSEL:
					soundmng_disable(SNDPROC_TOOL);
					r = dlgs_selectfile(hWnd, &skinui, np2tool.skin,
											sizeof(np2tool.skin), NULL);
					soundmng_enable(SNDPROC_TOOL);
					if (r) {
						skinchange(hWnd);
					}
					break;

				case IDM_SKINDEF:
					np2tool.skin[0] = '\0';
					skinchange(hWnd);
					break;

				case IDM_SKINMRU + 0:
				case IDM_SKINMRU + 1:
				case IDM_SKINMRU + 2:
				case IDM_SKINMRU + 3:
					file_cpyname(np2tool.skin,
									np2tool.skinmru[LOWORD(wp) - IDM_SKINMRU],
									sizeof(np2tool.skin));
					skinchange(hWnd);
					break;

				case IDM_TOOLCLOSE:
					SendMessage(hWnd, WM_CLOSE, 0, 0);
					break;

				default:
					if (!winui_en) {
						return(SendMessage(hWndMain, msg, wp, lp));
					}
					break;
			}
			break;

		case WM_KEYDOWN:						// TABを押した時に復帰
			if ((short)wp == VK_TAB) {
				idc = GetWindowLong(hWnd, GTWL_FOCUS);
				if (idc < IDC_MAXITEMS) {
					SetFocus(toolwin.sub[idc]);
				}
				return(0);
			}
			return(SendMessage(hWndMain, msg, wp, lp));

		case WM_KEYUP:
			if ((short)wp == VK_TAB) {
				return(0);
			}
			return(SendMessage(hWndMain, msg, wp, lp));

		case WM_PAINT:
			toolwinpaint(hWnd);
			break;

		case WM_DRAWITEM:
			tooldrawbutton(hWnd, wp, (LPDRAWITEMSTRUCT)lp);
			break;

		case WM_ENTERMENULOOP:
			soundmng_disable(SNDPROC_TOOL);
			break;

		case WM_EXITMENULOOP:
			soundmng_enable(SNDPROC_TOOL);
			break;

		case WM_ENTERSIZEMOVE:
			soundmng_disable(SNDPROC_TOOL);
			winlocex_destroy(toolwin.wlex);
			toolwin.wlex = np2_winlocexallwin(hWnd);
			break;

		case WM_MOVING:
			winlocex_moving(toolwin.wlex, (RECT *)lp);
			break;

		case WM_EXITSIZEMOVE:
			winlocex_destroy(toolwin.wlex);
			toolwin.wlex = NULL;
			soundmng_enable(SNDPROC_TOOL);
			break;

		case WM_MOVE:
			if (!(GetWindowLong(hWnd, GWL_STYLE) &
									(WS_MAXIMIZE | WS_MINIMIZE))) {
				RECT rc;
				GetWindowRect(hWnd, &rc);
				np2tool.posx = rc.left;
				np2tool.posy = rc.top;
				sysmng_update(SYS_UPDATEOSCFG);
			}
			break;

		case WM_CLOSE:
			sysmenu_settoolwin(0);
			sysmng_update(SYS_UPDATEOSCFG);
			DestroyWindow(hWnd);
			break;

		case WM_DESTROY:
			np2class_wmdestroy(hWnd);
			toolwindestroy();
			toolwin.hwnd = NULL;
			break;

		case WM_LBUTTONDOWN:
			if (np2tool.type & 1) {
				return(SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0L));
			}
			break;

		case WM_RBUTTONDOWN:
			openpopup(hWnd, lp);
			break;

		case WM_LBUTTONDBLCLK:
			np2tool.type ^= 1;
			wlex = np2_winlocexallwin(hWndMain);
			winlocex_setholdwnd(wlex, hWnd);
			np2class_windowtype(hWnd, (np2tool.type & 1) << 1);
			winlocex_move(wlex);
			winlocex_destroy(wlex);
			sysmng_update(SYS_UPDATEOSCFG);
			break;
#if defined(VAEG_EXT)
		case WM_MOUSEMOVE:
			checkmouseenter();
			startmousetracking();
			break;
		case WM_TIMER:
			checkmouseleave();
			checkquietdelaycount();
			break;
#endif
		default:
			return(DefWindowProc(hWnd, msg, wp, lp));
	}
	return(0);
}


// ---- 外向けAPI

BOOL toolwin_initapp(HINSTANCE hInstance) {

	WNDCLASS wc;

	wc.style = CS_BYTEALIGNCLIENT | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc = twproc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = GTWL_SIZE;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_TOOLWIN);
	wc.lpszClassName = np2toolclass;
	return(RegisterClass(&wc));
}

void toolwin_create(void) {

	HBITMAP	hbmp;
	BITMAP	bmp;
	HWND	hWnd;
/*
#if defined(VAEG_EXT)
	int		i;
#endif
*/
	if (toolwin.hwnd) {
		return;
	}
	ZeroMemory(&toolwin, sizeof(toolwin));
	hbmp = skinload(np2tool.skin);
	if (hbmp == NULL) {
		goto twope_err1;
	}
	GetObject(hbmp, sizeof(BITMAP), &bmp);
	toolwin.hbmp = hbmp;
	hWnd = CreateWindow(np2toolclass, np2tooltitle,
							WS_SYSMENU | WS_MINIMIZEBOX,
							np2tool.posx, np2tool.posy,
							bmp.bmWidth, bmp.bmHeight,
							NULL, NULL, hInst, NULL);
	winloc_setclientsize(hWnd, bmp.bmWidth, bmp.bmHeight);
	toolwin.hwnd = hWnd;
	if (hWnd == NULL) {
		goto twope_err2;
	}
	UpdateWindow(hWnd);
	ShowWindow(hWnd, SW_SHOWNOACTIVATE);
	SetForegroundWindow(hWndMain);

/*
#if defined(VAEG_EXT)
	// コマンドラインに指定したディスクイメージがtool windowに
	// 反映されるようにする。
	for (i = 0; i < FDDLIST_DRV; i++) {
		toolwin_setfdd(i, diskdrv_fname[i]); 
	}
#endif
*/

	return;

twope_err2:
	DeleteObject(hbmp);

twope_err1:
	sysmenu_settoolwin(0);
	sysmng_update(SYS_UPDATEOSCFG);
	return;
}

void toolwin_destroy(void) {

	if (toolwin.hwnd) {
		DestroyWindow(toolwin.hwnd);
	}
}

HWND toolwin_gethwnd(void) {

	return(toolwin.hwnd);
}

void toolwin_setfdd(BYTE drv, const char *name) {

	TOOLFDD	*fdd;
	char	*q;
	char	*p;
	UINT	i;
	HWND	sub;

	if (drv >= FDDLIST_DRV) {
		return;
	}
	fdd = np2tool.fdd + drv;
	if ((name == NULL) || (name[0] == '\0')) {
		fdd->insert = 0;
	}
	else {
		fdd->insert = 1;
		q = fdd->name[0];
		for (i=0; i<(FDDLIST_MAX - 1); i++) {
			if (!file_cmpname(q, name)) {
				break;
			}
			q += sizeof(fdd->name[0]);
		}
		p = q - sizeof(fdd->name[0]);
		while(i > 0) {
			i--;
			CopyMemory(q, p, sizeof(fdd->name[0]));
			p -= sizeof(fdd->name[0]);
			q -= sizeof(fdd->name[0]);
		}
		file_cpyname(fdd->name[0], name, sizeof(fdd->name[0]));
	}
	sysmng_update(SYS_UPDATEOSCFG);
	if (toolwin.hwnd != NULL) {
		sub = toolwin.sub[fddlist[drv]];
		if (sub) {
			remakefddlist(sub, fdd);
			SetForegroundWindow(hWndMain);
		}
#if defined(VAEG_EXT)
		InvalidateRect(toolwin.hwnd, NULL, TRUE);

		prominent();
		if (!toolwin.mouseover) quietsoon();
#endif
	}
}

#if defined(VAEG_EXT)
static void setdiskacc(UINT num, BYTE count, BYTE color) {
#else
static void setdiskacc(UINT num, BYTE count) {
#endif

const DISKACC	*acc;
	HWND		sub;

	if (toolwin.hwnd == NULL) {
		return;
	}
	if (num < (sizeof(diskacc)/sizeof(DISKACC))) {
		acc = diskacc + num;
		sub = NULL;
#if defined(VAEG_EXT)
		if (*(acc->counter) == 0 || *(acc->color) != color) {
#else
		if (*(acc->counter) == 0) {
#endif
			sub = toolwin.sub[acc->idc];
		}
		*(acc->counter) = count;
		if (sub) {
			InvalidateRect(sub, NULL, TRUE);
		}
#if defined(VAEG_EXT)
		*(acc->color) = color;
#endif
	}
}

#if defined(VAEG_EXT)
/*
FDDアクセスランプのON
	IN:
		drv = ドライ番号
		mediatype = 2HD なら1。それ以外は0
*/
void toolwin_fddaccess(BYTE drv, int mediatype) {

	if (drv < 2) {
//		setdiskacc(drv, 20, (mediatype) ? 2 : 0);
		setdiskacc(drv, 13, (mediatype) ? 2 : 0);
	}
}
#else
void toolwin_fddaccess(BYTE drv) {

	if (drv < 2) {
		setdiskacc(drv, 20);	
	}
}
#endif

void toolwin_hddaccess(BYTE drv) {
#if defined(VAEG_EXT)
	setdiskacc(2, 10, 0);
#else
	setdiskacc(2, 10);
#endif
}

#if defined(SUPPORT_PC88VA)
/*
モードLEDのON
	IN:
		num		LEDの番号 0.V1, 1.V2, 3.V3
		sw		0.OFF, その他.ON
*/
void toolwin_modeled(BYTE num, BYTE sw) {
	static const WORD	idc[3] = {IDC_TOOLV1MODE, IDC_TOOLV2MODE, IDC_TOOLV3MODE};

	if (num < 3) {
		if (sw) sw = 0xff;
		if (np2tool.modeled[num] != sw) {
			np2tool.modeled[num] = sw;

			InvalidateRect(toolwin.sub[idc[num]], NULL, TRUE);
		}
	}
}
#endif

void toolwin_draw(BYTE frame) {

const DISKACC	*acc;
const DISKACC	*accterm;
	BYTE		counter;
	HWND		sub;

	if (toolwin.hwnd == NULL) {
		return;
	}
	if (!frame) {
		frame = 1;
	}
	acc = diskacc;
	accterm = acc + (sizeof(diskacc)/sizeof(DISKACC));
	while(acc < accterm) {
		counter = *acc->counter;
		if (counter) {
			if (counter <= frame) {
				*(acc->counter) = 0;
				sub = toolwin.sub[acc->idc];
				if (sub) {
					InvalidateRect(sub, NULL, TRUE);
				}
			}
			else {
				*(acc->counter) -= frame;
			}
		}
		acc++;
	}
}


// ----

static const char ini_title[] = "NP2 tool";

static const INITBL iniitem[] = {
	{"WindposX", INITYPE_SINT32,	&np2tool.posx,			0},
	{"WindposY", INITYPE_SINT32,	&np2tool.posy,			0},
	{"WindType", INITYPE_BOOL,		&np2tool.type,			0},
	{"SkinFile", INITYPE_STR,		np2tool.skin,			MAX_PATH},
	{"SkinMRU0", INITYPE_STR,		np2tool.skinmru[0],		MAX_PATH},
	{"SkinMRU1", INITYPE_STR,		np2tool.skinmru[1],		MAX_PATH},
	{"SkinMRU2", INITYPE_STR,		np2tool.skinmru[2],		MAX_PATH},
	{"SkinMRU3", INITYPE_STR,		np2tool.skinmru[3],		MAX_PATH},
	{"FD1NAME0", INITYPE_STR,		np2tool.fdd[0].name[0],	MAX_PATH},
	{"FD1NAME1", INITYPE_STR,		np2tool.fdd[0].name[1],	MAX_PATH},
	{"FD1NAME2", INITYPE_STR,		np2tool.fdd[0].name[2],	MAX_PATH},
	{"FD1NAME3", INITYPE_STR,		np2tool.fdd[0].name[3],	MAX_PATH},
	{"FD1NAME4", INITYPE_STR,		np2tool.fdd[0].name[4],	MAX_PATH},
	{"FD1NAME5", INITYPE_STR,		np2tool.fdd[0].name[5],	MAX_PATH},
	{"FD1NAME6", INITYPE_STR,		np2tool.fdd[0].name[6],	MAX_PATH},
	{"FD1NAME7", INITYPE_STR,		np2tool.fdd[0].name[7],	MAX_PATH},
	{"FD2NAME0", INITYPE_STR,		np2tool.fdd[1].name[0],	MAX_PATH},
	{"FD2NAME1", INITYPE_STR,		np2tool.fdd[1].name[1],	MAX_PATH},
	{"FD2NAME2", INITYPE_STR,		np2tool.fdd[1].name[2],	MAX_PATH},
	{"FD2NAME3", INITYPE_STR,		np2tool.fdd[1].name[3],	MAX_PATH},
	{"FD2NAME4", INITYPE_STR,		np2tool.fdd[1].name[4],	MAX_PATH},
	{"FD2NAME5", INITYPE_STR,		np2tool.fdd[1].name[5],	MAX_PATH},
	{"FD2NAME6", INITYPE_STR,		np2tool.fdd[1].name[6],	MAX_PATH},
	{"FD2NAME7", INITYPE_STR,		np2tool.fdd[1].name[7],	MAX_PATH}};

void toolwin_readini(void) {

	char	path[MAX_PATH];

	ZeroMemory(&np2tool, sizeof(np2tool));
	np2tool.posx = CW_USEDEFAULT;
	np2tool.posy = CW_USEDEFAULT;
	np2tool.type = 1;
	initgetfile(path, sizeof(path));
	ini_read(path, ini_title, iniitem, sizeof(iniitem)/sizeof(INITBL));
}

void toolwin_writeini(void) {

	char	path[MAX_PATH];

	initgetfile(path, sizeof(path));
	ini_write(path, ini_title, iniitem, sizeof(iniitem)/sizeof(INITBL));
}

