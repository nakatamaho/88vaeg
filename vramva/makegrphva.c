/*
 * MAKEGRPHVA.C: PC-88VA Graphics
 */

#include	"compiler.h"
#include	"scrndraw.h"
#include	"gvramva.h"
#include	"videova.h"
#include	"makegrphva.h"

enum {
	GRPHVA_SCREENS	= 2,
};
typedef struct {
	UINT16	y;						// ���ݏ������̃��X�^(�O���t�B�b�N��ʂ̍��W�n��)
//	BOOL	r200lines;				// �𑜓x200/204���C���Ȃ�TRUE
	BOOL	r320dots;				// �𑜓x320�h�b�g�Ȃ�TRUE
	int		pixelmode;				// 0..1bit, 1..4bit, 2..8bit, 3..16bit
									// bit3 0..�p���b�g 1..���ڐF�w��
	WORD	*rasterbuf;
	BOOL	*noraster;
	UINT32	addrmask;
	UINT32	addrofs;

	UINT32	lineaddr;				// ����\��GVRAM�A�h���X
	UINT32	wrappedaddr;			// �������b�v�A���E���h���GVRAM�A�h���X
	UINT16	wrapcount;				// �������b�v�A���E���h����܂ł̎c��o�C�g��
	UINT16	vwrapcount;				// �������b�v�A���E���h����܂ł̎c�胉�C����
	FRAMEBUFFER	framebuffer;		// ���ݕ\�����Ă���t���[���o�b�t�@
	int		nextframebuffer;		// ���Ɏg�p����t���[���o�b�t�@�̔ԍ��B�����ꍇ��-1
} _SCREEN, *SCREEN;

typedef struct {
	UINT16		screeny;			// ���ݏ������̃��X�^(��ʋ��ʂ̍��W�n��)
	_SCREEN		screen[GRPHVA_SCREENS];
} _GRPHVAWORK;


static	_GRPHVAWORK	work;

		WORD grph0_raster[SURFACE_WIDTH + 64];
		WORD grph1_raster[SURFACE_WIDTH + 64];
											// 1���X�^���̃s�N�Z���f�[�^
											// �e�s�N�Z���̓J���[�R�[�h(16bit) �܂���
											// �p���b�g�ԍ�
											// ��ʉ��� + 
											// �ő�4�o�C�g(ex.1bit/pixel�Ȃ�32dot,
											// �����𑜓x320�Ȃ炻�̔{)��
											// �g�p����
		BOOL grph0_noraster;
		BOOL grph1_noraster;
											// ���蓖�Ă�ꂽ������ʂ��Ȃ��ꍇ�A
											// �O���t�B�b�N��ʔ�\���̏ꍇ�A
											// true

/*
static	WORD byte2pixel[256][8];			// �}���`�v���[��
											// 1�o�C�g��8�s�N�Z���ϊ��e�[�u��
*/
/*
static	WORD plane2pixel[4][4][4][4][2];	// �}���`�v���[��
											// 2bit*4plane��2�s�N�Z���ϊ��e�[�u��
*/
static	BYTE byte2pixel[256][8];			// �}���`�v���[��
											// 1�o�C�g��8�s�N�Z���ϊ��e�[�u��


#define addr18(scrn, x) ( (x) & ((scrn)->addrmask) | ((scrn)->addrofs) )
#define issingleplane() (videova.grmode & 0x0400)

static void selectframe(SCREEN screen, int no) {
	FRAMEBUFFER	f;

	if (no < 0 || no >= VIDEOVA_FRAMEBUFFERS) {
		screen->framebuffer = NULL;
		return;
	}

	f = &videova.framebuffer[no];
	screen->framebuffer = f;
	if (issingleplane()) {
		// �V���O���v���[�����[�h
		screen->lineaddr = addr18(screen, f->dsa);
		screen->wrappedaddr = addr18(screen, f->dsa - f->ofx);
	}
	else {
		// �}���`�v���[�����[�h
		screen->addrofs = (f->fsa & 0x20000L) ? 0x00020000L/4 : 0;
		screen->lineaddr = addr18(screen, f->dsa/4);
		screen->wrappedaddr = addr18(screen, f->dsa/4 - f->ofx/4);
	}
	if (f->fbl == 0xffff) {
		// screen 1 (�������b�v�A���E���h�Ȃ�)
		screen->vwrapcount = 0;
	}
	else {
		screen->vwrapcount = f->fbl + 1 - f->ofy;
	}

	if (no == 0) {
		screen->nextframebuffer = 2;
	}
	else if (no == 2) {
		screen->nextframebuffer = 3;
	}
	else {
		screen->nextframebuffer = -1;
	}
}

/*
���̃t���[���o�b�t�@��I������screen->framebuffer�ɐݒ肷��B
�ȉ��̏ꍇ��screen->framebuffer == NULL�ƂȂ�B
  �܂�DSP > screen->y �̏ꍇ
  DSP < screen->y �̏ꍇ (�㑱�t���[���o�b�t�@�S�Ă������ƂȂ�)
  �㑱�̃t���[���o�b�t�@���Ȃ��ꍇ
*/
static void selectnextframe(SCREEN screen) {
	screen->framebuffer = NULL;
	if (screen->nextframebuffer < 0) return;

	if (videova.framebuffer[screen->nextframebuffer].dsp < screen->y) {
		// �������̃t���[���o�b�t�@�͕\�����Ȃ�
		screen->nextframebuffer = -1;
	}
	else if (videova.framebuffer[screen->nextframebuffer].dsp == screen->y) {
		selectframe(screen, screen->nextframebuffer);
	}
}

static void endraster(SCREEN screen) {
	screen->vwrapcount--;
	if (screen->vwrapcount == 0) {
		// �������b�v�A���E���h
		screen->wrappedaddr = addr18(screen, screen->framebuffer->fsa);
		screen->lineaddr = addr18(screen, screen->wrappedaddr + screen->framebuffer->ofx);
	}
	else {
		screen->lineaddr = addr18(screen, screen->lineaddr + screen->framebuffer->fbw);
		screen->wrappedaddr = addr18(screen, screen->wrappedaddr + screen->framebuffer->fbw);
	}
}

static void endraster_m(SCREEN screen) {
	screen->vwrapcount--;
	if (screen->vwrapcount == 0) {
		// �������b�v�A���E���h
		screen->wrappedaddr = addr18(screen, screen->framebuffer->fsa/4);
		screen->lineaddr = addr18(screen, screen->wrappedaddr + screen->framebuffer->ofx/4);
	}
	else {
		screen->lineaddr = addr18(screen, screen->lineaddr + screen->framebuffer->fbw/4);
		screen->wrappedaddr = addr18(screen, screen->wrappedaddr + screen->framebuffer->fbw/4);
	}
}


// �V���O���v���[��1bit/pixel
static void drawraster_s1(SCREEN screen) {
	UINT16		xp;
	UINT16		wrapcount;
	UINT32		addr;
	WORD		*b;
	DWORD		dd;
	BYTE		fg;
	UINT16		i;

	addr = screen->lineaddr;
	b = screen->rasterbuf;
	if (screen->framebuffer->ofx == 0xffff) {
		// screen 1 (���b�v�A���E���h�Ȃ�)
		wrapcount = 0;
	}
	else {
		wrapcount = screen->framebuffer->fbw - screen->framebuffer->ofx;
	}

	// �t�H�A�O���E���h�J���[�̃p���b�g�ԍ�
	fg = (videova.pagemsk & 0x0f00) >> 8;


	if (screen->r320dots) {
		// 320 dots
		dd = ((DWORD)grphmem[addr+0] << 24) | 
			 ((DWORD)grphmem[addr+1] << 16) | 
			 ((DWORD)grphmem[addr+2] << 8) | 
			 grphmem[addr+3];
		addr = addr18(screen, addr + 4);

		i = screen->framebuffer->dot & 0x1f;
		dd <<= i;
		for (; i < 32; i++) {
			b[0] = b[1] = (dd & 0x80000000L) ? fg : 0;
			b += 2;
			dd <<= 1;
		}
		for (xp = 0; xp < 320/32; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			dd = ((DWORD)grphmem[addr+0] << 24) | 
				 ((DWORD)grphmem[addr+1] << 16) | 
				 ((DWORD)grphmem[addr+2] << 8) | 
				 grphmem[addr+3];
			addr = addr18(screen, addr + 4);

			for (i = 0; i < 32; i++) {
				b[0] = b[1] = (dd & 0x80000000L) ? fg : 0;
				b += 2;
				dd <<= 1;
			}
		}



	}
	else {
		// 640 dots

		dd = ((DWORD)grphmem[addr+0] << 24) | 
			 ((DWORD)grphmem[addr+1] << 16) | 
			 ((DWORD)grphmem[addr+2] << 8) | 
			 grphmem[addr+3];
		addr = addr18(screen, addr + 4);

		i = screen->framebuffer->dot & 0x1f;
		dd <<= i;
		for (; i < 32; i++) {
			*b++ = (dd & 0x80000000L) ? fg : 0;
			dd <<= 1;
		}
		for (xp = 0; xp < 640/32; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			dd = ((DWORD)grphmem[addr+0] << 24) | 
				 ((DWORD)grphmem[addr+1] << 16) | 
				 ((DWORD)grphmem[addr+2] << 8) | 
				 grphmem[addr+3];
			addr = addr18(screen, addr + 4);

			for (i = 0; i < 32; i++) {
				*b++ = (dd & 0x80000000L) ? fg : 0;
				dd <<= 1;
			}
		}
	}

	endraster(screen);
}


// �V���O���v���[��4bit/pixel
static void drawraster_s4(SCREEN screen) {
	UINT16		xp;
	UINT16		wrapcount;
	UINT32		addr;
	WORD		*b;

	WORD		d, d2;

	addr = screen->lineaddr;
	b = screen->rasterbuf;
	if (screen->framebuffer->ofx == 0xffff) {
		// screen 1 (���b�v�A���E���h�Ȃ�)
		wrapcount = 0;
	}
	else {
		wrapcount = screen->framebuffer->fbw - screen->framebuffer->ofx;
	}


	if (screen->r320dots) {
		// 320 dots
		d = LOADINTELWORD(grphmem + addr);
		d2 = LOADINTELWORD(grphmem + addr + 2);
		addr = addr18(screen, addr + 4);

		switch (screen->framebuffer->dot & 0x13) {
		case 0:
			b[0] = b[1] = (d >>  4) & 0x0f;
			b += 2;
		case 1:
			b[0] = b[1] = (d      ) & 0x0f;
			b += 2;
		case 2:
			b[0] = b[1] = (d >> 12) & 0x0f;
			b += 2;
		case 3:
			b[0] = b[1] = (d >>  8) & 0x0f;
			b += 2;
		case 0x10:
			b[0] = b[1] = (d2 >>  4) & 0x0f;
			b += 2;
		case 0x11:
			b[0] = b[1] = (d2      ) & 0x0f;
			b += 2;
		case 0x12:
			b[0] = b[1] = (d2 >> 12) & 0x0f;
			b += 2;
		case 0x13:
			b[0] = b[1] = (d2 >>  8) & 0x0f;
			b += 2;
		}
		for (xp = 0; xp < 320/8; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			d = LOADINTELWORD(grphmem + addr);
			d2 = LOADINTELWORD(grphmem + addr + 2);
			addr = addr18(screen, addr + 4);

			b[0] = b[1] = (d >>  4) & 0x0f;
			b += 2;
			b[0] = b[1] = (d      ) & 0x0f;
			b += 2;
			b[0] = b[1] = (d >> 12) & 0x0f;
			b += 2;
			b[0] = b[1] = (d >>  8) & 0x0f;
			b += 2;

			b[0] = b[1] = (d2 >>  4) & 0x0f;
			b += 2;
			b[0] = b[1] = (d2      ) & 0x0f;
			b += 2;
			b[0] = b[1] = (d2 >> 12) & 0x0f;
			b += 2;
			b[0] = b[1] = (d2 >>  8) & 0x0f;
			b += 2;
		}
	}
	else {
		// 640 dots

		d = LOADINTELWORD(grphmem + addr);
		d2 = LOADINTELWORD(grphmem + addr + 2);
		addr = addr18(screen, addr + 4);

		switch (screen->framebuffer->dot & 0x13) {
		case 0:
			*b++ = (d >>  4) & 0x0f;
		case 1:
			*b++ = (d      ) & 0x0f;
		case 2:
			*b++ = (d >> 12) & 0x0f;
		case 3:
			*b++ = (d >>  8) & 0x0f;
		case 0x10:
			*b++ = (d2 >>  4) & 0x0f;
		case 0x11:
			*b++ = (d2      ) & 0x0f;
		case 0x12:
			*b++ = (d2 >> 12) & 0x0f;
		case 0x13:
			*b++ = (d2 >>  8) & 0x0f;
		}
		for (xp = 0; xp < 640/8; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			d = LOADINTELWORD(grphmem + addr);
			d2 = LOADINTELWORD(grphmem + addr + 2);
			addr = addr18(screen, addr + 4);

			*b++ = (d >>  4) & 0x0f;
			*b++ = (d      ) & 0x0f;
			*b++ = (d >> 12) & 0x0f;
			*b++ = (d >>  8) & 0x0f;

			*b++ = (d2 >>  4) & 0x0f;
			*b++ = (d2      ) & 0x0f;
			*b++ = (d2 >> 12) & 0x0f;
			*b++ = (d2 >>  8) & 0x0f;
		}
	}

	endraster(screen);
}

// �V���O���v���[��8bit/pixel
static void drawraster_s8(SCREEN screen) {
	UINT16		xp;
	UINT16		wrapcount;
	UINT32		addr;
	WORD		*b;

	WORD		d, d2;

	addr = screen->lineaddr;
	b = screen->rasterbuf;
	if (screen->framebuffer->ofx == 0xffff) {
		// screen 1 (���b�v�A���E���h�Ȃ�)
		wrapcount = 0;
	}
	else {
		wrapcount = screen->framebuffer->fbw - screen->framebuffer->ofx;
	}


	if (screen->r320dots) {
		// 320 dots
		d = LOADINTELWORD(grphmem + addr);
		d2 = LOADINTELWORD(grphmem + addr + 2);
		addr = addr18(screen, addr + 4);

		switch (screen->framebuffer->dot & 0x11) {
		case 0:
			b[0] = b[1] = (d     ) & 0xff;
			b += 2;
		case 1:
			b[0] = b[1] = (d >> 8) & 0xff;
			b += 2;
		case 0x10:
			b[0] = b[1] = (d2     ) & 0xff;
			b += 2;
		case 0x11:
			b[0] = b[1] = (d2 >> 8) & 0xff;
			b += 2;
		}
		for (xp = 0; xp < 320/4; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			d = LOADINTELWORD(grphmem + addr);
			d2 = LOADINTELWORD(grphmem + addr + 2);
			addr = addr18(screen, addr + 4);
			b[0] = b[1] = (d     ) & 0xff;
			b += 2;
			b[0] = b[1] = (d >> 8) & 0xff;
			b += 2;

			b[0] = b[1] = (d2     ) & 0xff;
			b += 2;
			b[0] = b[1] = (d2 >> 8) & 0xff;
			b += 2;
		}
	}
	else {
		// 640 dots

		d = LOADINTELWORD(grphmem + addr);
		d2 = LOADINTELWORD(grphmem + addr + 2);
		addr = addr18(screen, addr + 4);

		switch (screen->framebuffer->dot & 0x11) {
		case 0:
			*b++ = (d     ) & 0xff;
		case 1:
			*b++ = (d >> 8) & 0xff;
		case 0x10:
			*b++ = (d2     ) & 0xff;
		case 0x11:
			*b++ = (d2 >> 8) & 0xff;
		}
		for (xp = 0; xp < 640/4; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			d = LOADINTELWORD(grphmem + addr);
			d2 = LOADINTELWORD(grphmem + addr + 2);
			addr = addr18(screen, addr + 4);
			*b++ = (d     ) & 0xff;
			*b++ = (d >> 8) & 0xff;

			*b++ = (d2     ) & 0xff;
			*b++ = (d2 >> 8) & 0xff;
		}
	}
	endraster(screen);
}

// �V���O���v���[��16bit/pixel
static void drawraster_s16(SCREEN screen) {
	UINT16		xp;
	UINT16		wrapcount;
	UINT32		addr;
	WORD		*b;

	WORD		d, d2;

	addr = screen->lineaddr;
	b = screen->rasterbuf;
	if (screen->framebuffer->ofx == 0xffff) {
		// screen 1 (���b�v�A���E���h�Ȃ�)
		wrapcount = 0;
	}
	else {
		wrapcount = screen->framebuffer->fbw - screen->framebuffer->ofx;
	}


	if (screen->r320dots) {
		// 320 dots
		d = LOADINTELWORD(grphmem + addr);
		d2 = LOADINTELWORD(grphmem + addr + 2);
		addr = addr18(screen, addr + 4);

		switch (screen->framebuffer->dot & 0x10) {
		case 0:
			b[0] = b[1] = d;
			b += 2;
		case 0x10:
			b[0] = b[1] = d2;
			b += 2;
		}

		for (xp = 0; xp < 320/2; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			d = LOADINTELWORD(grphmem + addr);
			d2 = LOADINTELWORD(grphmem + addr + 2);
			addr = addr18(screen, addr + 4);

			b[0] = b[1] = d;
			b += 2;
			b[0] = b[1] = d2;
			b += 2;
		}
	}
	else {
		// 640 dots

		d = LOADINTELWORD(grphmem + addr);
		d2 = LOADINTELWORD(grphmem + addr + 2);
		addr = addr18(screen, addr + 4);

		switch (screen->framebuffer->dot & 0x10) {
		case 0:
			*b++ = d;
		case 0x10:
			*b++ = d2;
		}
		for (xp = 0; xp < 640/2; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			d = LOADINTELWORD(grphmem + addr);
			d2 = LOADINTELWORD(grphmem + addr + 2);
			addr = addr18(screen, addr + 4);

			*b++ = d;
			*b++ = d2;
		}
	}
	endraster(screen);
}


// �}���`�v���[��4bit/pixel
static void drawraster_m4(SCREEN screen) {
//	UINT16		xp;
	UINT16		wrapcount;
	UINT32		addr;
	WORD		*b;
//	DWORD		dd;
	BYTE		d0, d1, d2, d3;
//	BYTE		fg;
	UINT16		i;

	UINT32		wrappedaddr;
	UINT32		addrmask;
	UINT32		addrofs;

	addr = screen->lineaddr;
	b = screen->rasterbuf;
	ZeroMemory(b, sizeof(grph0_raster));
	if (screen->framebuffer->ofx == 0xffff) {
		// screen 1 (���b�v�A���E���h�Ȃ�)
		wrapcount = 0;
	}
	else {
		wrapcount = screen->framebuffer->fbw/4 - screen->framebuffer->ofx/4;
	}

	// �t�H�A�O���E���h�J���[�̃p���b�g�ԍ�
	//fg = (videova.pagemsk & 0x0f00) >> 8;


	if (screen->r320dots) {
		i = screen->framebuffer->dot & 0x07;
		if (i > 0) {
			d0 = grphmem[addr];
			d1 = grphmem[addr + 0x10000];
			d2 = grphmem[addr + 0x20000];
			d3 = grphmem[addr + 0x30000];
			addr = addr18(screen, addr + 1);

			d0 <<= i;
			d1 <<= i;
			d2 <<= i;
			d3 <<= i;
			for (; i < 8; i++) {
				b[0] = b[1] = ((d0 & 0x80) >> 7) | 
					          ((d1 & 0x80) >> 6) | 
					          ((d2 & 0x80) >> 5) | 
					          ((d3 & 0x80) >> 4);
				d0 <<= 1;
				d1 <<= 1;
				d2 <<= 1;
				d3 <<= 1;
				b += 2;
			}
			wrapcount--;
		}

		wrappedaddr = screen->wrappedaddr;
		addrmask = screen->addrmask;
		addrofs  = screen->addrofs;
		__asm {
			mov		ecx, 320/8
			movzx	esi, wrapcount
			mov		edi, addr

loop3_0:	or		esi, esi
			jnz		loop3_1

			; if wrapcount == 0
			mov		edi, wrappedaddr;
loop3_1:
			dec		esi

			movzx	ebx, grphmem[edi + 0x30000]
			mov		eax, dword ptr byte2pixel[ebx*8]
			mov		edx, dword ptr byte2pixel[ebx*8+4]
			shl		eax, 1
			shl		edx, 1
			movzx	ebx, grphmem[edi + 0x20000]
			or		eax, dword ptr byte2pixel[ebx*8]
			or		edx, dword ptr byte2pixel[ebx*8+4]
			shl		eax, 1
			shl		edx, 1
			movzx	ebx, grphmem[edi + 0x10000]
			or		eax, dword ptr byte2pixel[ebx*8]
			or		edx, dword ptr byte2pixel[ebx*8+4]
			shl		eax, 1
			shl		edx, 1
			movzx	ebx, grphmem[edi + 0x00000]
			or		eax, dword ptr byte2pixel[ebx*8]
			or		edx, dword ptr byte2pixel[ebx*8+4]

			mov		ebx, b

			mov		[ebx+0], al
			mov		[ebx+2], al
			mov		[ebx+4], ah
			mov		[ebx+6], ah
			shr		eax, 16
			mov		[ebx+8], al
			mov		[ebx+10], al
			mov		[ebx+12], ah
			mov		[ebx+14], ah
			mov		[ebx+16], dl
			mov		[ebx+18], dl
			mov		[ebx+20], dh
			mov		[ebx+22], dh
			shr		edx, 16
			mov		[ebx+24], dl
			mov		[ebx+26], dl
			mov		[ebx+28], dh
			mov		[ebx+30], dh
			add		ebx, 8*2*2

			mov		b, ebx

			;addr = addr18(screen, addr + 1);
			inc		edi
			and		edi, addrmask
			or		edi, addrofs

			dec		cx
			jnz		loop3_0
		}

	
/*
		// 320 dots
		dd = ((DWORD)grphmem[addr+0] << 24) | 
			 ((DWORD)grphmem[addr+1] << 16) | 
			 ((DWORD)grphmem[addr+2] << 8) | 
			 grphmem[addr+3];
		addr = addr18(screen, addr + 4);

		i = screen->framebuffer->dot & 0x1f;
		dd <<= i;
		for (; i < 32; i++) {
			b[0] = b[1] = (dd & 0x80000000L) ? fg : 0;
			b += 2;
			dd <<= 1;
		}
		for (xp = 0; xp < 320/32; xp++) {
			wrapcount -= 4;
			if (wrapcount == 0) {
				addr = screen->wrappedaddr;
			}

			dd = ((DWORD)grphmem[addr+0] << 24) | 
				 ((DWORD)grphmem[addr+1] << 16) | 
				 ((DWORD)grphmem[addr+2] << 8) | 
				 grphmem[addr+3];
			addr = addr18(screen, addr + 4);

			for (i = 0; i < 32; i++) {
				b[0] = b[1] = (dd & 0x80000000L) ? fg : 0;
				b += 2;
				dd <<= 1;
			}
		}
*/


	}
	else {
		// 640 dots
		i = screen->framebuffer->dot & 0x07;
		if (i > 0) {
			d0 = grphmem[addr];
			d1 = grphmem[addr + 0x10000];
			d2 = grphmem[addr + 0x20000];
			d3 = grphmem[addr + 0x30000];
			addr = addr18(screen, addr + 1);

			d0 <<= i;
			d1 <<= i;
			d2 <<= i;
			d3 <<= i;
			for (; i < 8; i++) {
				*b++ = ((d0 & 0x80) >> 7) | 
					   ((d1 & 0x80) >> 6) | 
					   ((d2 & 0x80) >> 5) | 
					   ((d3 & 0x80) >> 4);
				d0 <<= 1;
				d1 <<= 1;
				d2 <<= 1;
				d3 <<= 1;
			}
			wrapcount--;
		}
#if 1
		wrappedaddr = screen->wrappedaddr;
		addrmask = screen->addrmask;
		addrofs  = screen->addrofs;
		__asm {
			mov		ecx, 640/8
			movzx	esi, wrapcount
			mov		edi, addr

loop_0:		or		esi, esi
			jnz		loop_1

			; if wrapcount == 0
			mov		edi, wrappedaddr;
loop_1:
			dec		esi

			movzx	ebx, grphmem[edi + 0x30000]
			mov		eax, dword ptr byte2pixel[ebx*8]
			mov		edx, dword ptr byte2pixel[ebx*8+4]
			shl		eax, 1
			shl		edx, 1
			movzx	ebx, grphmem[edi + 0x20000]
			or		eax, dword ptr byte2pixel[ebx*8]
			or		edx, dword ptr byte2pixel[ebx*8+4]
			shl		eax, 1
			shl		edx, 1
			movzx	ebx, grphmem[edi + 0x10000]
			or		eax, dword ptr byte2pixel[ebx*8]
			or		edx, dword ptr byte2pixel[ebx*8+4]
			shl		eax, 1
			shl		edx, 1
			movzx	ebx, grphmem[edi + 0x00000]
			or		eax, dword ptr byte2pixel[ebx*8]
			or		edx, dword ptr byte2pixel[ebx*8+4]

			mov		ebx, b

			mov		[ebx+0], al
			mov		[ebx+2], ah
			shr		eax, 16
			mov		[ebx+4], al
			mov		[ebx+6], ah
			mov		[ebx+8], dl
			mov		[ebx+10], dh
			shr		edx, 16
			mov		[ebx+12], dl
			mov		[ebx+14], dh
			add		ebx, 8*2

			mov		b, ebx

			;addr = addr18(screen, addr + 1);
			inc		edi
			and		edi, addrmask
			or		edi, addrofs

			dec		cx
			jnz		loop_0
		}
#else
		for (xp = 0; xp < 640/8; xp++) {
			//wrapcount -= 1;
			if (wrapcount-- == 0) {
				addr = screen->wrappedaddr;
			}

			/*
			d0 = grphmem[addr];
			d1 = grphmem[addr + 0x10000];
			d2 = grphmem[addr + 0x20000];
			d3 = grphmem[addr + 0x30000];
			addr = addr18(screen, addr + 1);
			*/
			
			__asm {
				mov		edi, addr
				movzx	ebx, grphmem[edi + 0x30000]
				mov		eax, dword ptr byte2pixel[ebx*8]
				mov		edx, dword ptr byte2pixel[ebx*8+4]
				shl		eax, 1
				shl		edx, 1
				movzx	ebx, grphmem[edi + 0x20000]
				or		eax, dword ptr byte2pixel[ebx*8]
				or		edx, dword ptr byte2pixel[ebx*8+4]
				shl		eax, 1
				shl		edx, 1
				movzx	ebx, grphmem[edi + 0x10000]
				or		eax, dword ptr byte2pixel[ebx*8]
				or		edx, dword ptr byte2pixel[ebx*8+4]
				shl		eax, 1
				shl		edx, 1
				movzx	ebx, grphmem[edi + 0x00000]
				or		eax, dword ptr byte2pixel[ebx*8]
				or		edx, dword ptr byte2pixel[ebx*8+4]
				mov		ebx, b
				mov		[ebx+0], al
				;mov		[ebx+1], byte ptr 0
				mov		[ebx+2], ah
				;mov		[ebx+3], byte ptr 0
				shr		eax, 16
				mov		[ebx+4], al
				;mov		[ebx+5], byte ptr 0
				mov		[ebx+6], ah
				;mov		[ebx+7], byte ptr 0
				mov		[ebx+8], dl
				;mov		[ebx+9], byte ptr 0
				mov		[ebx+10], dh
				;mov		[ebx+11], byte ptr 0
				shr		edx, 16
				mov		[ebx+12], dl
				;mov		[ebx+13], byte ptr 0
				mov		[ebx+14], dh
				;mov		[ebx+15], byte ptr 0
			}
			
			/*
			__asm {
				mov		edi, addr
				movzx	ebx, grphmem[edi + 0x30000]
				shl		ebx, 3
				mov		eax, dword ptr byte2pixel[ebx]
				mov		edx, dword ptr byte2pixel[ebx+4]
				shl		eax, 1
				shl		edx, 1
				movzx	ebx, grphmem[edi + 0x20000]
				shl		ebx, 3
				or		eax, dword ptr byte2pixel[ebx]
				or		edx, dword ptr byte2pixel[ebx+4]
				shl		eax, 1
				shl		edx, 1
				movzx	ebx, grphmem[edi + 0x10000]
				shl		ebx, 3
				or		eax, dword ptr byte2pixel[ebx]
				or		edx, dword ptr byte2pixel[ebx+4]
				shl		eax, 1
				shl		edx, 1
				movzx	ebx, grphmem[edi + 0x00000]
				shl		ebx, 3
				or		eax, dword ptr byte2pixel[ebx]
				or		edx, dword ptr byte2pixel[ebx+4]
				mov		ebx, b
				mov		[ebx+0], al
				mov		[ebx+2], ah
				shr		eax, 16
				mov		[ebx+4], al
				mov		[ebx+6], ah
				mov		[ebx+8], dl
				mov		[ebx+10], dh
				shr		edx, 16
				mov		[ebx+12], dl
				mov		[ebx+14], dh
			}
			*/
			addr = addr18(screen, addr + 1);
			b += 8;

/*
			for (i = 0; i < 8; i++) {
				*b++ = ((d0 & 0x80) >> 7) | 
					   ((d1 & 0x80) >> 6) | 
					   ((d2 & 0x80) >> 5) | 
					   ((d3 & 0x80) >> 4);
				d0 <<= 1;
				d1 <<= 1;
				d2 <<= 1;
				d3 <<= 1;
			}
*/
/*
			{
				DWORD p;

				p = *((DWORD *)&byte2pixel[d0][0]);
				p |= *((DWORD *)&byte2pixel[d1][0]) << 1;
				p |= *((DWORD *)&byte2pixel[d2][0]) << 2;
				p |= *((DWORD *)&byte2pixel[d3][0]) << 3;
				*((DWORD *)b) = p;
				b += 2;

				p = *((DWORD *)&byte2pixel[d0][2]);
				p |= *((DWORD *)&byte2pixel[d1][2]) << 1;
				p |= *((DWORD *)&byte2pixel[d2][2]) << 2;
				p |= *((DWORD *)&byte2pixel[d3][2]) << 3;
				*((DWORD *)b) = p;
				b += 2;

				p = *((DWORD *)&byte2pixel[d0][4]);
				p |= *((DWORD *)&byte2pixel[d1][4]) << 1;
				p |= *((DWORD *)&byte2pixel[d2][4]) << 2;
				p |= *((DWORD *)&byte2pixel[d3][4]) << 3;
				*((DWORD *)b) = p;
				b += 2;

				p = *((DWORD *)&byte2pixel[d0][6]);
				p |= *((DWORD *)&byte2pixel[d1][6]) << 1;
				p |= *((DWORD *)&byte2pixel[d2][6]) << 2;
				p |= *((DWORD *)&byte2pixel[d3][6]) << 3;
				*((DWORD *)b) = p;
				b += 2;
			}
*/
/*
			{
				for (i = 6; i < 8 ; i-=2) {
					*((DWORD *)&b[i]) = 
						*((DWORD *)&plane2pixel[d0&3][d1&3][d2&3][d3&3][0]);
					d0 >>= 2;
					d1 >>= 2;
					d2 >>= 2;
					d3 >>= 2;
				}
				b += 8;
			}
*/
		}
#endif
	}

	endraster_m(screen);
}


static void drawraster(SCREEN screen) {

//	if (!screen->r200lines || (work.screeny & 1) == 0) {
		if (screen->framebuffer != NULL) {
			if (screen->framebuffer->dsp + screen->framebuffer->dsh == screen->y) {
				screen->framebuffer = NULL;
			}
		}

		// �K�v�Ȃ�A���̃t���[���o�b�t�@�ɐ؂�ւ���
		if (screen->framebuffer == NULL) {
			do {
				selectnextframe(screen);
			} while (!(screen->framebuffer == NULL || screen->framebuffer->dsh > 0 ));
		}

		//if (screen->rasterbuf) {
			*screen->noraster = FALSE;
			if (screen->framebuffer == NULL) {
				// �����\�����Ȃ�
				/*
				UINT16		xp;
				WORD		*b;
				b = screen->rasterbuf;
				for (xp = 0; xp < 640; xp++) *b++ = 0;
				*/
				*screen->noraster = TRUE;
			}
			else if (issingleplane()) {
				// �V���O���v���[�����[�h
				switch (screen->pixelmode) {
				case 0:
					drawraster_s1(screen);
					break;
				case 1:
					drawraster_s4(screen);
					break;
				case 2:
					drawraster_s8(screen);
					break;
				case 3:
					drawraster_s16(screen);
					break;
				}
			}
			else {
				// �}���`�v���[�����[�h
				switch (screen->pixelmode) {
				/*
				case 0:
					drawraster_m1(screen);
					break;
				*/
				case 1:
					drawraster_m4(screen);
					break;
				default:
					{
						// �����\�����Ȃ� (�����ł͂Ȃ��A0���o��)
						UINT16		xp;
						WORD		*b;
						b = screen->rasterbuf;
						for (xp = 0; xp < 640; xp++) *b++ = 0;
					}
					break;
				}
			}
		//}
		screen->y++;

//	}
}


		
void makegrphva_initialize(void) {
/*
	// �}���`�v���[�� 1�o�C�g��8�s�N�Z�� �ϊ��e�[�u��
	{
		int d;
		int i;

		for (d = 0; d < 256; d++) {
			int dat = d;
			for (i = 7; i >= 0; i--) {
				byte2pixel[d][i] = dat & 1;
				dat >>= 1;
			}
		}
	}
*/
/*
	// �}���`�v���[�� 2bit*4plane��2�s�N�Z���ϊ��e�[�u��
	{
		int p0,p1,p2,p3;
		for (p0 = 0; p0 < 4; p0++) {
			for (p1 = 0; p1 < 4; p1++) {
				for (p2 = 0; p2 < 4; p2++) {
					for (p3 = 0; p3 < 4; p3++) {
						plane2pixel[p0][p1][p2][p3][1] = 
							(p0 & 1) | 
							((p1 & 1) << 1) |
							((p2 & 1) << 2) |
							((p3 & 1) << 3);
						plane2pixel[p0][p1][p2][p3][0] = 
							((p0 & 2) >> 1) | 
							((p1 & 2)     ) |
							((p2 & 2) << 1) |
							((p3 & 2) << 2);
					}
				}
			}
		}
	}
*/
	// �}���`�v���[�� 1�o�C�g��8�s�N�Z�� �ϊ��e�[�u��
	{
		int d;
		int i;

		for (d = 0; d < 256; d++) {
			int dat = d;
			for (i = 7; i >= 0; i--) {
				byte2pixel[d][i] = dat & 1;
				dat >>= 1;
			}
		}
	}
}

void makegrphva_begin(BOOL *scrn200) {

	work.screeny = 0;

	work.screen[0].pixelmode = videova.grres & 0x0003;
	work.screen[1].pixelmode = (videova.grres >> 8) & 0x0003;
	work.screen[0].rasterbuf = grph0_raster;
	work.screen[1].rasterbuf = grph1_raster;
	work.screen[0].noraster  = &grph0_noraster;
	work.screen[1].noraster  = &grph1_noraster;

	work.screen[0].r320dots = videova.grres & 0x0010;
	work.screen[1].r320dots = videova.grres & 0x1000;
//	work.screen[0].r200lines = videova.grmode & 0x0002;
//	work.screen[1].r200lines = videova.grmode & 0x0002;
	if (issingleplane()) {
		if (videova.grmode & 0x0800)  {
			// 2��ʃ��[�h
			work.screen[0].addrmask = 0x0001ffffL;
		}
		else {
			// 1��ʃ��[�h
			work.screen[0].addrmask = 0x0003ffffL;
		}
		work.screen[0].addrofs  = 0x00000000L;
	}
	else {
		work.screen[0].addrmask = 0x0001ffffL/4;
	}
	work.screen[1].addrmask = 0x0001ffffL;
	work.screen[1].addrofs  = 0x00020000L;
	work.screen[0].nextframebuffer = 0;
	work.screen[1].nextframebuffer = 1;
	work.screen[0].y = 0;
	work.screen[1].y = 0;
	selectframe(&work.screen[0], -1);
	selectframe(&work.screen[1], -1);

	*scrn200 = videova.grmode & 0x0002;
}

void makegrphva_blankraster(void) {
	int i;
//	UINT16 xp;

	for (i = 0; i < GRPHVA_SCREENS; i++) {
		/*
		for (xp = 0; xp < SURFACE_WIDTH; xp++) {
			work.screen[i].rasterbuf[xp] = 0;
		}
		*/
		*work.screen[i].noraster = TRUE;
	}
}

void makegrphva_raster(void) {

	if (!(videova.grmode & 0x8000)) {
		// �O���t�B�b�N�\���֎~
		makegrphva_blankraster();
	}
	else if (videova.grmode & 0x0400) {
		// �V���O���v���[�����[�h
		drawraster(&work.screen[0]);
		drawraster(&work.screen[1]);
	}
	else {
		// �}���`�v���[�����[�h
		drawraster(&work.screen[0]);
	}
	
	work.screeny++;
}
