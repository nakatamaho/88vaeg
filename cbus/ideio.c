#include	"compiler.h"

// winでidentifyまでは取得に行くんだけどな…ってAnex86も同じか

#if defined(SUPPORT_IDEIO)

#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"ideio.h"
#include	"atapicmd.h"
#include	"sxsi.h"
#include	"idebios.res"


	IDEIO	ideio;


static IDEDEV getidedev(void) {

	UINT	bank;

	bank = ideio.bank[1] & 0x7f;
	if (bank < 2) {
		return(ideio.dev + bank);
	}
	else {
		return(NULL);
	}
}

static IDEDRV getidedrv(void) {

	IDEDEV	dev;
	IDEDRV	drv;

	dev = getidedev();
	if (dev) {
		drv = dev->drv + dev->drivesel;
		if (drv->device != IDETYPE_NONE) {
			return(drv);
		}
	}
	return(NULL);
}

static const char serial[] = "824919341192        ";
static const char firm[] = "A5U.1200";
static const char model[] = "QUANTUM FIREBALL CR                     ";

static BOOL setidentify(IDEDRV drv) {

	SXSIDEV sxsi;
	UINT16	tmp[256];
	BYTE	*p;
	UINT	i;
	UINT32	size;

	sxsi = sxsi_getptr(drv->sxsidrv);
	if ((sxsi == NULL) || (sxsi->fname[0] == '\0')) {
		return(FAILURE);
	}

	ZeroMemory(tmp, sizeof(tmp));
	// とりあえず使ってる部分だけ
	tmp[0] = 0x0040;
	tmp[1] = sxsi->cylinders;
	tmp[3] = sxsi->surfaces;
	tmp[4] = sxsi->sectors * 512;
	tmp[6] = sxsi->sectors;
	for (i=0; i<10; i++) {
		tmp[10+i] = (serial[i*2] << 8) + serial[i*2+1];
	}
	tmp[22] = 4;
	for (i=0; i<4; i++) {
		tmp[23+i] = (firm[i*2] << 8) + firm[i*2+1];
	}
	for (i=0; i<20; i++) {
		tmp[27+i] = (model[i*2] << 8) + model[i*2+1];
	}
#if IDEIO_MULTIPLE_MAX > 0
	tmp[47] = 0x8000 | IDEIO_MULTIPLE_MAX;	// multiple
#endif
	tmp[49] = 0x0000;			// LBA(1 << 9)
	tmp[51] = 0x0200;
	tmp[53] = 0x0001;
	tmp[54] = sxsi->cylinders;
	tmp[55] = sxsi->surfaces;
	tmp[56] = sxsi->sectors;
	size = sxsi->cylinders * sxsi->surfaces * sxsi->sectors;
	tmp[57] = (UINT16)size;
	tmp[58] = (UINT16)(size >> 16);
#if IDEIO_MULTIPLE_MAX > 0
	tmp[59] = 0x0100 | drv->mulmode;	// current multiple mode
#endif
	tmp[60] = (UINT16)size;
	tmp[61] = (UINT16)(size >> 16);
	tmp[63] = 0x0000;			// no support multiword DMA

	tmp[80] = 0x0006;			// only support ATA-1/2
	tmp[81] = 0;
	tmp[82] = 0x0000;			// DEVICE RESET(1 << 9)

	p = drv->buf;
	for (i=0; i<256; i++) {
		p[0] = (BYTE)tmp[i];
		p[1] = (BYTE)(tmp[i] >> 8);
		p += 2;
	}
	drv->bufdir = IDEDIR_IN;
	drv->bufpos = 0;
	drv->bufsize = 512;
	return(SUCCESS);
}

static void setintr(IDEDRV drv) {

	if (!(drv->ctrl & IDECTRL_NIEN)) {
		TRACEOUT(("ideio: setintr()"));
		ideio.bank[0] = ideio.bank[1] | 0x80;			// ????
		pic_setirq(IDE_IRQ);
	}
}

static void cmdabort(IDEDRV drv) {

	TRACEOUT(("ideio: cmdabort()"));
	drv->status = IDESTAT_DRDY | IDESTAT_ERR;
	drv->error = IDEERR_ABRT;
	setintr(drv);
}

static void panic(const char *str, ...) {

	char	buf[2048];
	va_list	ap;

	va_start(ap, str);
	vsnprintf(buf, sizeof(buf), str, ap);
	va_end(ap);

	msgbox("ide_panic", buf);
	exit(1);
}


// ----

static void incsec(IDEDRV drv) {

	if (!(drv->dr & IDEDEV_LBA)) {
		drv->sn++;
		if (drv->sn <= drv->sectors) {
			return;
		}
		drv->sn = 1;
		drv->hd++;
		if (drv->hd < drv->surfaces) {
			return;
		}
		drv->hd = 0;
		drv->cy++;
	}
	else {
		TRACEOUT(("ideio: incsec() LBA mode?"));
		drv->sn++;
		if (drv->sn) {
			return;
		}
		drv->cy++;
		if (drv->cy) {
			return;
		}
		drv->hd++;
	}
}

static long getcursec(const IDEDRV drv) {

	long	ret;

	if (!(drv->dr & IDEDEV_LBA)) {
		ret = drv->cy;
		ret *= drv->surfaces;
		ret += drv->hd;
		ret *= drv->sectors;
		ret += (drv->sn - 1);
	}
	else {
		TRACEOUT(("ideio: getcursec() LBA mode?"));
		ret = drv->sn;
		ret |= (drv->cy << 8);
		ret |= (drv->hd << 24);
	}
	return(ret);
}

static void readsec(IDEDRV drv) {

	long	sec;

	if (!drv->device) {
		goto read_err;
	}
	sec = getcursec(drv);
	TRACEOUT(("readsec->drv %d sec %x cnt %d thr %d",
								drv->sxsidrv, sec, drv->mulcnt, drv->multhr));
	if (sxsi_read(drv->sxsidrv, sec, drv->buf, 512)) {
		TRACEOUT(("read error!"));
		goto read_err;
	}
	drv->bufdir = IDEDIR_IN;
	drv->bufpos = 0;
	drv->bufsize = 512;

	if ((drv->mulcnt & (drv->multhr - 1)) == 0) {
		drv->status = IDESTAT_DRDY | IDESTAT_DSC | IDESTAT_DRQ;
		drv->error = 0;
		setintr(drv);
	}
	drv->mulcnt++;
	return;

read_err:
	cmdabort(drv);
}


// ----

static void IOOUTCALL ideio_o430(UINT port, REG8 dat) {

	TRACEOUT(("ideio setbank%d %.2x [%.4x:%.8x]",
									(port >> 1) & 1, dat, CPU_CS, CPU_EIP));
	if (!(dat & 0x80)) {
		ideio.bank[(port >> 1) & 1] = dat;
	}
}

static REG8 IOINPCALL ideio_i430(UINT port) {

	UINT	bank;
	REG8	ret;

	bank = (port >> 1) & 1;
	ret = ideio.bank[bank];
	ideio.bank[bank] = ret & (~0x80);
	TRACEOUT(("ideio getbank%d %.2x [%.4x:%.8x]",
									(port >> 1) & 1, ret, CPU_CS, CPU_EIP));
	return(ret & 0x81);
}


// ----

static void IOOUTCALL ideio_o642(UINT port, REG8 dat) {

	IDEDRV	drv;

	drv = getidedrv();
	if (drv) {
		drv->wp = dat;
		TRACEOUT(("ideio set WP %.2x [%.4x:%.8x]", dat, CPU_CS, CPU_EIP));
	}
	(void)port;
}

static void IOOUTCALL ideio_o644(UINT port, REG8 dat) {

	IDEDRV	drv;

	(void)port;
	drv = getidedrv();
	if (drv) {
		drv->sc = dat;
		TRACEOUT(("ideio set SC %.2x [%.4x:%.8x]", dat, CPU_CS, CPU_EIP));
	}
	(void)port;
}

static void IOOUTCALL ideio_o646(UINT port, REG8 dat) {

	IDEDRV	drv;

	drv = getidedrv();
	if (drv) {
		drv->sn = dat;
		TRACEOUT(("ideio set SN %.2x [%.4x:%.8x]", dat, CPU_CS, CPU_EIP));
	}
	(void)port;
}

static void IOOUTCALL ideio_o648(UINT port, REG8 dat) {

	IDEDRV	drv;

	drv = getidedrv();
	if (drv) {
		drv->cy &= 0xff00;
		drv->cy |= dat;
		TRACEOUT(("ideio set CYL %.2x [%.4x:%.8x]", dat, CPU_CS, CPU_EIP));
	}
	(void)port;
}

static void IOOUTCALL ideio_o64a(UINT port, REG8 dat) {

	IDEDRV	drv;

	drv = getidedrv();
	if (drv) {
		drv->cy &= 0x00ff;
		drv->cy |= dat << 8;
		TRACEOUT(("ideio set CYH %.2x [%.4x:%.8x]", dat, CPU_CS, CPU_EIP));
	}
	(void)port;
}

static void IOOUTCALL ideio_o64c(UINT port, REG8 dat) {

	IDEDEV	dev;
	UINT	drvnum;

	dev = getidedev();
	if (dev == NULL) {
		return;
	}
#if defined(TRACE)
	if ((dat & 0xf0) != 0xa0) {
		TRACEOUT(("ideio set SDH illegal param? (%.2x)", dat));
	}
#endif
	drvnum = (dat >> 4) & 1;
	dev->drivesel = drvnum;
	dev->drv[drvnum].dr = dat & 0xf0;
	dev->drv[drvnum].hd = dat & 0x0f;
	TRACEOUT(("ideio set DRHD %.2x [%.4x:%.8x]", dat, CPU_CS, CPU_EIP));
	(void)port;
}

static void IOOUTCALL ideio_o64e(UINT port, REG8 dat) {

	IDEDRV	drv;
	IDEDEV	dev;

	// execute device diagnostic
	if (dat == 0x90) {
		TRACEOUT(("ideio set cmd %.2x [%.4x:%.8x]", dat, CPU_CS, CPU_EIP));
		TRACEOUT(("ideio: execute device diagnostic"));
		dev = getidedev();
		if (dev) {
			IDEDRV d;
			int i;

			for (i = 0; i < 2; i++) {
				d = dev->drv + i;
				d->hd = 0x00;
				d->sc = 0x01;
				d->sn = 0x01;
				d->cy = 0x0000;
				d->status = IDESTAT_DRDY;
				d->error = 0x01;
				if (i == 0) {
					if (dev->drv[0].device == IDETYPE_NONE) {
						d->error = 0x00;
					}
					if (dev->drv[1].device == IDETYPE_NONE) {
						d->error |= 0x80;
					}
				}
				else {
					if (dev->drv[1].device == IDETYPE_NONE) {
						d->error = 0x00;
					}
				}
			}
		}
		return;
	}

	drv = getidedrv();
	if (drv == NULL) {
		return;
	}
	drv->cmd = dat;
	TRACEOUT(("ideio set cmd %.2x [%.4x:%.8x]", dat, CPU_CS, CPU_EIP));
	switch(dat) {
		case 0x08:		// device reset
			TRACEOUT(("ideio: device reset"));
			if (drv->device == IDETYPE_NONE) {
				cmdabort(drv);
				break;
			}
			else if (drv->device == IDETYPE_HDD) {
				drv->hd = 0x00;
				drv->sc = 0x01;
				drv->sn = 0x01;
				drv->cy = 0x0000;
			}
			else if (drv->device == IDETYPE_CDROM) {
				drv->hd = 0x10;
				drv->sc = 0x01;
				drv->sn = 0x01;
				drv->cy = 0xeb14;
			}
			drv->status = 0x00;
			drv->error = 0x01;
			dev = getidedev();
			if (dev) {
				if (dev->drivesel == 0) {
					if (dev->drv[0].device == IDETYPE_NONE) {
						drv->error = 0x00;
					}
					if (dev->drv[1].device == IDETYPE_NONE) {
						drv->error |= 0x80;
					}
				}
				else {
					if (dev->drv[1].device == IDETYPE_NONE) {
						drv->error = 0x00;
					}
				}
			}
			setintr(drv);
			break;

		case 0x10:		// calibrate
//		case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:
//		case 0x16: case 0x17: case 0x18: case 0x19: case 0x1a:
//		case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
			TRACEOUT(("ideio: calibrate"));
			if (drv->device) {
				drv->status = IDESTAT_DRDY | IDESTAT_DSC;
				drv->error = 0;
			}
			else {
				drv->status = IDESTAT_DRDY | IDESTAT_ERR;
				drv->error = IDEERR_TR0;
			}
			setintr(drv);
			break;

		case 0x20:		// read (with retry)
		case 0x21:		// read
			TRACEOUT(("ideio: read sector"));
			drv->mulcnt = 0;
			drv->multhr = 1;
			readsec(drv);
			break;

		case 0x91:		// set parameters
			TRACEOUT(("ideio: set parameters dh=%x sec=%x",
											drv->dr | drv->hd, drv->sc));
			if (drv->device != IDETYPE_NONE) {
				drv->surfaces = drv->hd + 1;
				drv->sectors = drv->sc;
				drv->status &= ~(IDESTAT_BSY | IDESTAT_DRQ | IDESTAT_ERR);
				drv->status |= IDESTAT_DRDY;
				setintr(drv);
			}
			else {
				cmdabort(drv);
			}
			break;

		case 0xa0:		// send packet
			if (drv->device == IDETYPE_CDROM) {
				drv->status &= ~(IDESTAT_BSY | IDESTAT_DWF | IDESTAT_ERR);
				drv->status |= IDESTAT_DRQ;
				drv->error = 0;
				drv->bufpos = 0;
				drv->bufsize = 12;
				drv->bufdir = IDEDIR_OUT;
			}
			else {
				cmdabort(drv);
			}
			break;

		case 0xa1:		// identify
		case 0xec:		// identify
			TRACEOUT(("ideio: identify"));
			if (setidentify(drv) == SUCCESS) {
				drv->status = IDESTAT_DRDY | IDESTAT_DSC | IDESTAT_DRQ;
				drv->error = 0;
				setintr(drv);
			}
			else {
				cmdabort(drv);
			}
			break;

		case 0xc4:		// read multiple
#if IDEIO_MULTIPLE_MAX > 0
			TRACEOUT(("ideio: read multiple"));
			drv->mulcnt = 0;
			drv->multhr = drv->mulmode;
			readsec(drv);
#else
			cmdabort(drv);
#endif
			break;

		case 0xc6:		// set multiple mode
			TRACEOUT(("ideio: set multiple mode"));
			switch(drv->sc) {
#if IDEIO_MULTIPLE_MAX > 0
				case 2: case 4: case 8: case 16: case 32: case 64: case 128:
					if (drv->sc <= IDEIO_MULTIPLE_MAX) {
						drv->mulmode = drv->sc;
						setintr(drv);
						break;
					}
					/*FALLTHROUGH*/
#endif
				default:
					cmdabort(drv);
					break;
			}
			break;

		case 0xef:		// set features
			TRACEOUT(("ideio: set features reg = %.2x", drv->wp));
			cmdabort(drv);
			break;

		default:
			panic("ideio: unknown command %.2x", dat);
			break;
	}
	(void)port;
}

static void IOOUTCALL ideio_o74c(UINT port, REG8 dat) {

	IDEDEV	dev;
	REG8	modify;

	dev = getidedev();
	if (dev == NULL) {
		return;
	}
	modify = dev->drv[0].ctrl ^ dat;
	dev->drv[0].ctrl = dat;
	dev->drv[1].ctrl = dat;
	if (modify & IDECTRL_SRST) {
		if (dat & IDECTRL_SRST) {
			dev->drv[0].status = 0;
			dev->drv[0].error = 0;
			dev->drv[1].status = 0;
			dev->drv[1].error = 0;
		}
		else {
			if (dev->drv[0].device) {
				dev->drv[0].status = IDESTAT_DRDY | IDESTAT_DSC;
				dev->drv[0].error = IDEERR_AMNF;
			}
			if (dev->drv[1].device) {
				dev->drv[1].status = IDESTAT_DRDY | IDESTAT_DSC;
				dev->drv[1].error = IDEERR_AMNF;
			}
		}
	}
	TRACEOUT(("ideio interrupt %sable", (dat & IDECTRL_NIEN) ? "dis" : "en"));
	TRACEOUT(("ideio devctrl %.2x [%.4x:%.8x]", dat, CPU_CS, CPU_EIP));
	(void)port;
}

static void IOOUTCALL ideio_o74e(UINT port, REG8 dat) {

	TRACEOUT(("ideio %.4x,%.2x [%.4x:%.8x]", port, dat, CPU_CS, CPU_EIP));
	(void)port;
	(void)dat;
}


// ----

static REG8 IOINPCALL ideio_i642(UINT port) {

	IDEDRV	drv;

	(void)port;

	drv = getidedrv();
	if (drv) {
		drv->status &= ~IDESTAT_ERR;
		TRACEOUT(("ideio get error %.2x [%.4x:%.8x]",
												drv->error, CPU_CS, CPU_EIP));
		return(drv->error);
	}
	else {
		return(0xff);
	}
}

static REG8 IOINPCALL ideio_i644(UINT port) {

	IDEDRV	drv;

	(void)port;

	drv = getidedrv();
	if (drv) {
		TRACEOUT(("ideio get SC %.2x [%.4x:%.8x]", drv->sc, CPU_CS, CPU_EIP));
		return(drv->sc);
	}
	else {
		return(0xff);
	}
}

static REG8 IOINPCALL ideio_i646(UINT port) {

	IDEDRV	drv;

	(void)port;

	drv = getidedrv();
	if (drv) {
		TRACEOUT(("ideio get SN %.2x [%.4x:%.8x]", drv->sn, CPU_CS, CPU_EIP));
		return(drv->sn);
	}
	else {
		return(0xff);
	}
}

static REG8 IOINPCALL ideio_i648(UINT port) {

	IDEDRV	drv;

	(void)port;

	drv = getidedrv();
	if (drv) {
		TRACEOUT(("ideio get CYL %.4x [%.4x:%.8x]", drv->cy, CPU_CS, CPU_EIP));
		return((UINT8)drv->cy);
	}
	else {
		return(0xff);
	}
}

static REG8 IOINPCALL ideio_i64a(UINT port) {

	IDEDRV	drv;

	(void)port;

	drv = getidedrv();
	if (drv) {
		TRACEOUT(("ideio get CYH %.4x [%.4x:%.8x]", drv->cy, CPU_CS, CPU_EIP));
		return((REG8)(drv->cy >> 8));
	}
	else {
		return(0xff);
	}
}

static REG8 IOINPCALL ideio_i64c(UINT port) {

	IDEDRV	drv;
	REG8	ret;

	(void)port;

	drv = getidedrv();
	if (drv) {
		ret = drv->dr | drv->hd;
		TRACEOUT(("ideio get DRHD %.2x [%.4x:%.8x]", ret, CPU_CS, CPU_EIP));
		return(ret);
	}
	else {
		return(0xff);
	}
}

static REG8 IOINPCALL ideio_i64e(UINT port) {

	IDEDRV	drv;

	(void)port;

	drv = getidedrv();
	if (drv) {
		TRACEOUT(("ideio status %.2x [%.4x:%.8x]",
											drv->status, CPU_CS, CPU_EIP));
		if (!(drv->ctrl & IDECTRL_NIEN)) {
			TRACEOUT(("ideio: resetirq"));
			pic_resetirq(IDE_IRQ);
		}
		return(drv->status);
	}
	else {
		return(0xff);
	}
}

static REG8 IOINPCALL ideio_i74c(UINT port) {

	IDEDRV	drv;

	(void)port;

	drv = getidedrv();
	if (drv) {
		TRACEOUT(("ideio alt status %.2x [%.4x:%.8x]",
											drv->status, CPU_CS, CPU_EIP));
		return(drv->status);
	}
	else {
		return(0xff);
	}
}


// ---- data

void IOOUTCALL ideio_w16(UINT port, REG16 value) {

	IDEDRV	drv;
	BYTE	*p;

	drv = getidedrv();
	if ((drv != NULL) &&
		(drv->status & IDESTAT_DRQ) && (drv->bufdir == IDEDIR_OUT)) {
		p = drv->buf + drv->bufpos;
		p[0] = (BYTE)value;
		p[1] = (BYTE)(value >> 8);
		TRACEOUT(("ide-data send %4x (%.4x) [%.4x:%.8x]",
										value, drv->bufpos, CPU_CS, CPU_EIP));
		drv->bufpos += 2;
		if (drv->bufpos >= drv->bufsize) {
			drv->status &= ~IDESTAT_DRQ;
			switch(drv->cmd) {
				case 0xa0:
					atapicmd_a0(drv);
					break;
			}
		}
	}
	(void)port;
}

REG16 IOINPCALL ideio_r16(UINT port) {

	IDEDRV	drv;
	REG16	ret;
	BYTE	*p;

	(void)port;

	drv = getidedrv();
	if (drv == NULL) {
		return(0xff);
	}
	ret = 0;
	if ((drv->status & IDESTAT_DRQ) && (drv->bufdir == IDEDIR_IN)) {
		p = drv->buf + drv->bufpos;
		ret = p[0] + (p[1] << 8);
		TRACEOUT(("ide-data recv %.4x (%.4x) [%.4x:%.8x]",
										ret, drv->bufpos, CPU_CS, CPU_EIP));
		drv->bufpos += 2;
		if (drv->bufpos >= drv->bufsize) {
			drv->status &= ~IDESTAT_DRQ;
			switch(drv->cmd) {
				case 0x20:
				case 0x21:
				case 0xc4:
					incsec(drv);
					drv->sc--;
					if (drv->sc) {
						readsec(drv);
					}
					break;

				case 0xa0:
					drv->sc = IDEINTR_IO | IDEINTR_CD;
					drv->status &= ~(IDESTAT_BSY | IDESTAT_DRQ | IDESTAT_ERR);
					drv->status |= IDESTAT_DRDY;
					drv->error = 0;
					setintr(drv);
					break;
			}
		}
	}
	return(ret);
}


// ----

static void devinit(IDEDRV drv, REG8 sxsidrv) {

	SXSIDEV	sxsi;

	ZeroMemory(drv, sizeof(_IDEDRV));
	sxsi = sxsi_getptr(sxsidrv);
	if ((sxsi != NULL) && (sxsi->fname[0] != '\0')) {
		drv->sxsidrv = sxsidrv;
		drv->status = IDESTAT_DRDY | IDESTAT_DSC;
		drv->error = IDEERR_AMNF;
		drv->device = IDETYPE_HDD;
		drv->surfaces = sxsi->surfaces;
		drv->sectors = sxsi->sectors;
		drv->mulmode = 1;
	}
	else {
		drv->status = IDESTAT_ERR;
		drv->error = IDEERR_TR0;
		drv->device = IDETYPE_NONE;
	}
}

void ideio_reset(void) {

	REG8	i;
	IDEDRV	drv;

	ZeroMemory(&ideio, sizeof(ideio));
	for (i=0; i<4; i++) {
		drv = ideio.dev[i >> 1].drv + (i & 1);
		devinit(drv, i);
	}

#if 0		// CD-ROM test
	drv = ideio.dev[1].drv;
	drv->hd = 0;
	drv->sc = 1;
	drv->sn = 1;
	drv->cy = 0xeb14;
	drv->status = 0;
	drv->error = 0;
	drv->device = IDETYPE_CDROM;
#endif

#if 0
	ideio.dev[0].drv[0].status = IDE_READY | IDE_SEEKCOMPLETE;
	ideio.dev[0].drv[0].error = 1;
	ideio.dev[1].drv[0].status = IDE_READY | IDE_SEEKCOMPLETE;
	ideio.dev[1].drv[0].error = 1;

	ideio.dev[0].drv[0].sxsidrv = 0x00;
	ideio.dev[0].drv[1].sxsidrv = 0x01;
	ideio.dev[1].drv[0].sxsidrv = 0x02;
	ideio.dev[1].drv[1].sxsidrv = 0x03;
#endif

	CopyMemory(mem + 0xd0000, idebios, sizeof(idebios));
	TRACEOUT(("use simulate ide.rom"));
}

void ideio_bind(void) {

	if (pccore.hddif & PCHDD_IDE) {
		iocore_attachout(0x0430, ideio_o430);
		iocore_attachout(0x0432, ideio_o430);
		iocore_attachinp(0x0430, ideio_i430);
		iocore_attachinp(0x0432, ideio_i430);

		iocore_attachout(0x0642, ideio_o642);
		iocore_attachout(0x0644, ideio_o644);
		iocore_attachout(0x0646, ideio_o646);
		iocore_attachout(0x0648, ideio_o648);
		iocore_attachout(0x064a, ideio_o64a);
		iocore_attachout(0x064c, ideio_o64c);
		iocore_attachout(0x064e, ideio_o64e);
		iocore_attachinp(0x0642, ideio_i642);
		iocore_attachinp(0x0644, ideio_i644);
		iocore_attachinp(0x0646, ideio_i646);
		iocore_attachinp(0x0648, ideio_i648);
		iocore_attachinp(0x064a, ideio_i64a);
		iocore_attachinp(0x064c, ideio_i64c);
		iocore_attachinp(0x064e, ideio_i64e);

		iocore_attachout(0x074c, ideio_o74c);
		iocore_attachout(0x074e, ideio_o74e);
		iocore_attachinp(0x074c, ideio_i74c);
	}
}

#endif	/* SUPPORT_IDEIO */
