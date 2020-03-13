#include	"compiler.h"

#if defined(SUPPORT_SCSI)

#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"cbuscore.h"
#include	"scsiio.h"
#include	"scsiio.tbl"
#include	"scsicmd.h"
#include	"scsibios.res"

#if defined(SUPPORT_PC88VA)
#include	"iocoreva.h"
#endif


	_SCSIIO		scsiio;

static const UINT8 scsiirq[] = {0x03, 0x05, 0x06, 0x09, 0x0c, 0x0d, 3, 3};

#if defined(VAEG_EXT)
//TEST
static UINT8 nextstatus=0;

static void scsiintr(REG8 status) ;
static void scsitarget(NEVENTITEM item) {
		scsiintr(nextstatus);
		nextstatus=0;
}

static int gettransfercounter(void) {
	return (((int)scsiio.reg[SCSICTR_TRANSCNT + 0]) << 16) +
		   (((int)scsiio.reg[SCSICTR_TRANSCNT + 1]) << 8) +
		   (((int)scsiio.reg[SCSICTR_TRANSCNT + 2]) );
}

static void settransfercounter(int v) {
	scsiio.reg[SCSICTR_TRANSCNT + 0] = (v & 0xff0000) >> 16;
	scsiio.reg[SCSICTR_TRANSCNT + 1] = (v & 0x00ff00) >> 8;
	scsiio.reg[SCSICTR_TRANSCNT + 2] = (v & 0x0000ff);
}

static int dectransfercounter(void) {
	int c = gettransfercounter();
	c--;
	settransfercounter(c);
	return c;
}

#endif

void scsiioint(NEVENTITEM item) {

	TRACEOUT(("scsiioint"));
	if (scsiio.membank & 4) {
		pic_setirq(scsiirq[(scsiio.resent >> 3) & 7]);
		TRACEOUT(("scsi intr"));
	}
	scsiio.auxstatus = 0x80;
	(void)item;

#if defined(VAEG_EXT)
	if (nextstatus) {
		nevent_set(NEVENT_SCSIIO, 4000, scsitarget, NEVENT_ABSOLUTE);
		TRACEOUT(("scsi schedule target action"));
	}
#endif
}


static void scsiintr(REG8 status) {

	scsiio.scsistatus = status;
	nevent_set(NEVENT_SCSIIO, 4000, scsiioint, NEVENT_ABSOLUTE);
	TRACEOUT(("scsi schedule intr"));
}


#if defined(VAEG_EXT)

static void scsistartreceive(void) {
	UINT8 id;

	switch(scsiio.phase) {
		case SCSIPH_DATAIN:
		case SCSIPH_STATUS:
		case SCSIPH_INFOIN:
		case SCSIPH_MSGIN:
			break;
		default:
			return;
	}
	scsiio.rddatpos = 0;
	id = scsiio.reg[SCSICTR_DSTID] & 7;
	scsicmd_start_transferinfo_in(id);	// ��M�f�[�^��scsiio.data�ɓ���B
}

static REG8 scsireceivebyte(void) {
	REG8 dat;
	REG8 ret;

	dat = scsiio.data[scsiio.rddatpos & 0x7fff];

	switch(scsiio.phase) {
		case SCSIPH_DATAIN:
		case SCSIPH_STATUS:
		case SCSIPH_INFOIN:
		case SCSIPH_MSGIN:
			break;
		default:
			return dat;
	}

	if (gettransfercounter() > 0) {
		scsiio.rddatpos++;
		if (dectransfercounter() == 0) {
			TRACEOUT(("scsi: receive: transfer counter has decremented to 0"));
			switch(scsiio.reg[SCSICTR_CMD]) {
				case SCSICMD_TRANS_INFO:
					ret = scsicmd_end_transferinfo_in();
					if (ret != 0xff) {
						scsiintr(ret);
					}
					break;
			}
		}
	}

	return dat;
}


static void scsisendbyte(REG8 dat) {
	UINT8 id;
	REG8 ret;

	switch(scsiio.phase) {
		case SCSIPH_DATAOUT:
		case SCSIPH_COMMAND:
		case SCSIPH_INFOOUT:
		case SCSIPH_MSGOUT:
			break;
		default:
			return;
	}

	if (gettransfercounter() > 0) {
		if (scsiio.wrdatpos < sizeof(scsiio.data)) {
			scsiio.data[scsiio.wrdatpos++] = dat;
		}
		if (dectransfercounter() == 0) {
			TRACEOUT(("scsi: send: transfer counter has decremented to 0"));
			id = scsiio.reg[SCSICTR_DSTID] & 7;
			switch(scsiio.reg[SCSICTR_CMD]) {
				case SCSICMD_TRANS_INFO:
					ret = scsicmd_transferinfo_out(id, scsiio.data);
					if (ret != 0xff) {
						scsiintr(ret);
					}
					break;
			}
		}
	}
}

#endif


static void scsicmd(REG8 cmd) {

	REG8	ret;
	UINT8	id;

	id = scsiio.reg[SCSICTR_DSTID] & 7;
	switch(cmd) {
		case SCSICMD_RESET:
			scsiintr(SCSISTAT_RESET);
			break;

		case SCSICMD_NEGATE:
			ret = scsicmd_negate(id);
			scsiintr(ret);
			break;

		case SCSICMD_SEL:
			ret = scsicmd_select(id);
			if (ret & 0x80) {
				scsiintr(0x11);
				// �� ret�͂ǁ[����Ċ��荞�݂�����́H
#if defined(VAEG_EXT)
				nextstatus = ret;
#endif
			}
			else {
				scsiintr(ret);
			}
			break;

		case SCSICMD_SEL_TR:
			ret = scsicmd_transfer(id, scsiio.reg + SCSICTR_CDB);
			if (ret != 0xff) {
				scsiintr(ret);
			}
			break;
#if defined(VAEG_EXT)
		case SCSICMD_TRANS_INFO:
			// CPU->SCSI�]�����ASCSICTR_TRANSCNT�Ŏw�肳�ꂽ�T�C�Y�̃f�[�^��CPU����]������Ă���܂ő҂�
			// SCSI->CPU�]�����Ascsiio.data�̐擪����CPU���ǂݏo����悤�ɂ���
			scsiio.wrdatpos = 0;
			scsistartreceive();
			break;

		default:
			TRACEOUT(("scsi: unknown command %.2x", cmd));
			break;
#endif

	}
}




// ----

static void IOOUTCALL scsiio_occ0(UINT port, REG8 dat) {

	scsiio.port = dat;
	(void)port;
}

static void IOOUTCALL scsiio_occ2(UINT port, REG8 dat) {

	UINT8	bit;

	if (scsiio.port < 0x40) {
		TRACEOUT(("scsi ctrl write %s(%.2x) %.2x", scsictr[scsiio.port], scsiio.port, dat));
	}
	if (scsiio.port <= 0x19) {
		scsiio.reg[scsiio.port] = dat;
		if (scsiio.port == SCSICTR_CMD) {
			scsicmd(dat);
		}
#if defined(VAEG_EXT)
		else if (scsiio.port == SCSICTR_DATA) {
			scsisendbyte(dat);
			/*
			if (gettransfercounter() > 0) {
				if (scsiio.wrdatpos < sizeof(scsiio.data)) {
					scsiio.data[scsiio.wrdatpos++] = dat;
				}
				if (dectransfercounter() == 0) {
					UINT8 id;
					REG8 ret;

					TRACEOUT(("scsi: transfer counter has decremented to 0"));
					id = scsiio.reg[SCSICTR_DSTID] & 7;
					switch(scsiio.reg[SCSICTR_CMD]) {
						case SCSICMD_TRANS_INFO:
							ret = scsicmd_transferinfo(id, scsiio.data);
							if (ret != 0xff) {
								scsiintr(ret);
							}
							break;
					}
				}
			}
			*/
		}
#endif
	scsiio.port++;
	}
	else {
		switch(scsiio.port) {
			case SCSICTR_MEMBANK:
				scsiio.membank = dat;
				if (!(dat & 0x40)) {
					CopyMemory(mem + 0xd2000, scsiio.bios[0], 0x2000);
				}
				else {
					CopyMemory(mem + 0xd2000, scsiio.bios[1], 0x2000);
				}
				break;

			case 0x3f:
				bit = 1 << (dat & 7);
				if (dat & 8) {
					scsiio.datmap |= bit;
				}
				else {
					if (scsiio.datmap & bit) {
						scsiio.datmap &= ~bit;
						if (bit == (1 << 1)) {
							scsiio.wrdatpos = 0;
						}
						else if (bit == (1 << 5)) {
							scsiio.rddatpos = 0;
						}
					}
				}
				break;
		}
	}
	(void)port;
}

static void IOOUTCALL scsiio_occ4(UINT port, REG8 dat) {

	TRACEOUT(("scsiio_occ4 %.2x", dat));
	(void)port;
	(void)dat;
}

static void IOOUTCALL scsiio_occ6(UINT port, REG8 dat) {

	scsiio.data[scsiio.wrdatpos & 0x7fff] = dat;
	scsiio.wrdatpos++;
	(void)port;
}

static REG8 IOINPCALL scsiio_icc0(UINT port) {

	REG8	ret;
#if defined(VAEG_EXT)
	REG8	dbr;
#endif

	ret = scsiio.auxstatus;
	scsiio.auxstatus = 0;
#if defined(VAEG_EXT)
	dbr = 0;
	switch(scsiio.reg[SCSICTR_CMD]) {
		case SCSICMD_TRANS_INFO:
			if (gettransfercounter() > 0) {
				dbr = 1;
			}
			break;
	}
	ret = (ret & 0xfe) | dbr;
#endif
	(void)port;
	return(ret);
}

static REG8 IOINPCALL scsiio_icc2(UINT port) {

#if defined(VAEG_EXT)
	REG8	ret = 0xff;
	UINT	scsiioport = scsiio.port;

	switch(scsiio.port) {
		case SCSICTR_STATUS:
			scsiio.port++;
			ret = scsiio.scsistatus;
			break;

		case SCSICTR_MEMBANK:
			ret = scsiio.membank;
			break;

		case SCSICTR_MEMWND:
			ret = scsiio.memwnd;
			break;

		case SCSICTR_RESENT:
			ret = scsiio.resent;
			break;

		case 0x36:
			ret = 0;					// �Q���h���Ƃ��c
			break;

#if defined(VAEG_EXT)
		case SCSICTR_DATA:
			/*
			if (gettransfercounter() > 0) {
				scsiio.reg[SCSICTR_DATA] = scsiio.data[(scsiio.rddatpos++) & 0x7fff];
				if (dectransfercounter() == 0) {
					TRACEOUT(("scsi: transfer counter has decremented to 0"));
					switch(scsiio.reg[SCSICTR_CMD]) {
						case SCSICMD_TRANS_INFO:
							switch(scsiio.phase) {
								case SCSIPH_DATAIN:	// Transfer�R�}���h����I��(�X�e�[�^�X�t�F�[�Y)
									scsiintr(0x1b);
									scsiio.data[0] = 0; // Status
									scsiio.phase = SCSIPH_STATUS;
									break;
								case SCSIPH_STATUS:
									scsiintr(0x1f);	// Transfer�R�}���h����I��(ү���޲�̪���)
									scsiio.data[0] = 0; // Message
									scsiio.phase = SCSIPH_MSGIN;
									break;
								case SCSIPH_MSGIN:
									scsiintr(0x20);	// Transfer�R�}���h(ү���޲�̪���)��ACK�̃A�T�[�g��ԂŃ|�[�Y
													// ���b�Z�[�W�C���t�F�[�Y�������牽��Ԃ��ׂ�? 0x20?
									break;
							}
							break;
					}
				}
			}
			ret = scsiio.reg[SCSICTR_DATA];
			*/
			ret = scsireceivebyte();
			scsiio.port++;
			break;
#endif
		default:
			if (scsiio.port <= 0x19) {
				ret = scsiio.reg[scsiio.port];
				scsiio.port++;
			}
			break;
	}

	TRACEOUT(("scsi ctrl read %s(%.2x) %.2x [%.4x:%.4x]",
							scsictr[scsiioport], scsiioport, ret, CPU_CS, CPU_IP));
	(void)port;
	return(ret);

#else
	REG8	ret;

	switch(scsiio.port) {
		case SCSICTR_STATUS:
			scsiio.port++;
			return(scsiio.scsistatus);

		case SCSICTR_MEMBANK:
			return(scsiio.membank);

		case SCSICTR_MEMWND:
			return(scsiio.memwnd);

		case SCSICTR_RESENT:
			return(scsiio.resent);

		case 0x36:
			return(0);					// �Q���h���Ƃ��c
	}
	if (scsiio.port <= 0x19) {
		ret = scsiio.reg[scsiio.port];
		TRACEOUT(("scsi ctrl read %s %.2x [%.4x:%.4x]",
							scsictr[scsiio.port], ret, CPU_CS, CPU_IP));
		scsiio.port++;
		return(ret);
	}
	(void)port;
	return(0xff);
#endif
}

static REG8 IOINPCALL scsiio_icc4(UINT port) {

	TRACEOUT(("scsiio_icc4"));
	(void)port;
	return(0x00);
}

static REG8 IOINPCALL scsiio_icc6(UINT port) {

	REG8	ret;

	ret = scsiio.data[scsiio.rddatpos & 0x7fff];
	scsiio.rddatpos++;
	(void)port;
	return(ret);
}


// ----

void scsiio_reset(void) {

	FILEH	fh;
	UINT	r;

	ZeroMemory(&scsiio, sizeof(scsiio));
	if (pccore.hddif & PCHDD_SCSI) {
		scsiio.memwnd = (0xd200 & 0x0e00) >> 9;
#if defined(VAEG_EXT) //SASI�ƏՓ˂�������邽�߂̉����u
		scsiio.resent = (2 << 3) + (7 << 0);
#else
		scsiio.resent = (3 << 3) + (7 << 0);
#endif
		CPU_RAM_D000 |= (3 << 2);				// ram�ɂ���
		fh = file_open_rb_c("scsi.rom");
		r = 0;
		if (fh != FILEH_INVALID) {
			r = file_read(fh, scsiio.bios, 0x4000);
			file_close(fh);
		}
		if (r == 0x4000) {
			TRACEOUT(("load scsi.rom"));
		}
		else {
			ZeroMemory(mem + 0xd2000, 0x4000);
			CopyMemory(scsiio.bios, scsibios, sizeof(scsibios));
			TRACEOUT(("use simulate scsi.rom"));
		}
		CopyMemory(mem + 0xd2000, scsiio.bios[0], 0x2000);
	}
}

void scsiio_bind(void) {

	if (pccore.hddif & PCHDD_SCSI) {
		iocore_attachout(0x0cc0, scsiio_occ0);
		iocore_attachout(0x0cc2, scsiio_occ2);
		iocore_attachout(0x0cc4, scsiio_occ4);
		iocore_attachout(0x0cc6, scsiio_occ6);
		iocore_attachinp(0x0cc0, scsiio_icc0);
		iocore_attachinp(0x0cc2, scsiio_icc2);
		iocore_attachinp(0x0cc4, scsiio_icc4);
		iocore_attachinp(0x0cc6, scsiio_icc6);
#if defined(SUPPORT_PC88VA)
		iocoreva_attachout(0x0cc0, scsiio_occ0);
		iocoreva_attachout(0x0cc2, scsiio_occ2);
		iocoreva_attachout(0x0cc4, scsiio_occ4);
		iocoreva_attachinp(0x0cc0, scsiio_icc0);
		iocoreva_attachinp(0x0cc2, scsiio_icc2);
		iocoreva_attachinp(0x0cc4, scsiio_icc4);
#endif
	}
}

#endif

