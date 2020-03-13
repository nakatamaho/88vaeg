/*
 * FDSUBSYS.C: PC-88VA FD Sub System (mock-up type)
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"memoryva.h"
#include	"fddfile.h"

#if defined(SUPPORT_PC88VA)

enum {
	// �|�[�gC �̃r�b�g(�T�u�V�X�e������̌����ꍇ)

	//   ���C�����T�u
	ATN_MAIN = 0x08,	//�R�}���h�o�͒�		attention
	DAC_MAIN = 0x04,	//�f�[�^����芮��		data accepted
	RFD_MAIN = 0x02,	//�n���h�V�F�C�N���f�B	ready for data
	DAV_MAIN = 0x01,	//�f�[�^�o�͒�			data valid

	//   �T�u�����C��
	DAC_SUB  = 0x40,
	RFD_SUB  = 0x20,
	DAV_SUB  = 0x10,

	DACBIT   = 6,
	RFDBIT   = 5,
	DAVBIT   = 4,

	// �T�u�̃n���h�V�F�C�N�̏��
	HSST_STOPPED		= 0,
	HSST_WAIT_ATN		= 1,	// ATN�҂�
	HSST_WAIT_CMD		= 2,	// DAV�҂�/�R�}���h��M�҂�
	HSST_WAIT_DATA		= 3,	// DAV�҂�/�f�[�^��M�҂�
	HSST_WAIT_DAV_RESET	= 4,	// DAV�����҂�
	HSST_WAIT_RFD		= 11,	// RFD�҂�/�f�[�^���M�҂�
	HSST_WAIT_DAC		= 12,	// DAC�҂�
	HSST_WAIT_DAC_RESET	= 13,	// DAC�����҂�

	// �T�u�̃R�}���h���s�T�C�N���̏��
	ST_RECV_CMD			= 0,
	ST_RECV_DATA		= 1,
	ST_EXEC_CMD			= 2,
	ST_SEND_DATA		= 3,
	ST_END_CYCLE		= 4,

	WAITING		= 0,
	GOAHEAD		= 1,

	// �T�u�V�X�e���R�}���h
	CMD_INITIALIZE			= 0x00,
	CMD_WRITE_DATA			= 0x01,
	CMD_READ_DATA			= 0x02,
	CMD_SEND_DATA			= 0x03,
	CMD_SEND_RESULT_STATUS	= 0x06,
	CMD_RECEIVE_MEMORY		= 0x0c,
	CMD_EXECUTE_COMMAND		= 0x0d,
	CMD_LOAD_DATA			= 0x0e,
	CMD_SET_SURFACE_MODE	= 0x17,
	CMD_SET_DISK_MODE		= 0x1f,
	CMD_SEND_DISK_MODE		= 0x20,
	CMD_SET_BOUNDARY_MODE	= 0x21,
	CMD_DRIVE_READY_CHECK	= 0x23,
	CMD_SLEEP				= 0x25,
	CMD_ACTIVE				= 0x26,

	// �T�u�V�X�e�����[�N�������̃A�h���X
	WORK_DATA_BUF			= 0x4000,		// ���[�h/���C�g�o�b�t�@
	WORK_READ_SECTOR_COUNT	= 0x7f08,		// ���[�h�����Z�N�^��(���[�h�R�}���h���s��)
	WORK_COMMAND_STATUS		= 0x7f14,		// �R�}���h�X�e�[�^�X�B(�R�}���h6�Q��)
	WORK_DISK_MODE			= 0x7f44,		// �f�B�X�N���[�h �h���C�u0,1�̏�
	WORK_LAST_DISK_MODE		= 0x7f4f,		// �Ō�ɃA�N�Z�X�����h���C�u�̃f�B�X�N���[�h
	WORK_DM_N				= 0x7f52,		// �f�B�X�N���[�h�ɑΉ�����FDC��N
	

	// ���̑�
	DATA_BUF_SIZE			= 0x2000,		// ���[�h/���C�g�o�b�t�@�̃T�C�Y
	DRIVES					= 2,			// �h���C�u��
};

static const BYTE ntobit[]={0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

static BYTE porta_main;
static BYTE portb_main;
static BYTE portc_main;


// ---- �T�u�V�X�e��

		BYTE subsysmem[0x10000];	// �T�u�V�X�e��������(64K�o�C�g)

static int	hsstate;		// �n���h�V�F�C�N�̏��
static int	state;

static BYTE	cmd;
static BOOL cmdrecvd;		// �R�}���h��M�ς�

static int	recvdatacnt;	// ��M����f�[�^�̃o�C�g�� ���Z�J�E���^
static BYTE *recvbuf;		// ��M�o�b�t�@�ւ̃|�C���^
static int	senddatacnt;	// ���M����f�[�^�̃o�C�g�� ���Z�J�E���^
static BYTE *sendbuf;		// ���M�f�[�^�ւ̃|�C���^

static BYTE	parambuf[8];		// �p�����[�^����M�o�b�t�@


static void subsys_outportb(REG8 dat) {
	porta_main = dat;
}

static REG8 subsys_inporta(void) {
	return portb_main;
}

static REG8 subsys_inportc(void) {
	return (portc_main << 4) | ((portc_main >> 4) & 0x0f);
}

static void subsys_setportc(int bitnum) {
	if (bitnum >= 4) {
		portc_main |= ntobit[bitnum-4];
	}
}

static void subsys_resetportc(int bitnum) {
	if (bitnum >= 4) {
		portc_main &= ~ntobit[bitnum-4];
	}
}

static int subsys_wait_atn(void) {
	if (subsys_inportc() & ATN_MAIN) {
		subsys_setportc(RFDBIT);
		hsstate = HSST_WAIT_CMD;
		return GOAHEAD;
	}
	return WAITING;
}

static int subsys_wait_cmd(void) {
	if (subsys_inportc() & DAV_MAIN) {
		subsys_resetportc(RFDBIT);
		cmd = subsys_inporta();
		cmdrecvd = TRUE;
		subsys_setportc(DACBIT);
		hsstate = HSST_WAIT_DAV_RESET;
		return GOAHEAD;
	}
	return WAITING;
}

static int subsys_wait_data(void) {
	BYTE data;
	if (subsys_inportc() & DAV_MAIN) {
		subsys_resetportc(RFDBIT);
		data = subsys_inporta();
//		TRACEOUT(("fdsubsys: recv data 0x%02x", data));
		*recvbuf++ = data;
		recvdatacnt--;
		subsys_setportc(DACBIT);
		hsstate = HSST_WAIT_DAV_RESET;
		return GOAHEAD;
	}
	return WAITING;
}

static int subsys_wait_dav_reset(void) {
	if (!(subsys_inportc() & DAV_MAIN)) {
		subsys_resetportc(DACBIT);
		hsstate = HSST_STOPPED;
		return GOAHEAD;
	}
	return WAITING;
}

static int subsys_wait_rfd(void) {
	BYTE data;
	if (subsys_inportc() & RFD_MAIN) {
		data = *sendbuf++;
		senddatacnt--;
		subsys_outportb(data);
//		TRACEOUT(("fdsubsys: send data 0x%02x", data));
		subsys_setportc(DAVBIT);
		hsstate = HSST_WAIT_DAC;
		return GOAHEAD;
	}
	return WAITING;
}

static int subsys_wait_dac(void) {
	if (subsys_inportc() & DAC_MAIN) {
		subsys_resetportc(DAVBIT);
		hsstate = HSST_WAIT_DAC_RESET;
		return GOAHEAD;
	}
	return WAITING;
}

static int subsys_wait_dac_reset(void) {
	if (!(subsys_inportc() & DAC_MAIN)) {
		hsstate = HSST_STOPPED;
		return GOAHEAD;
	}
	return WAITING;
}


static int subsys_receive_cmd(void) {
	int result;

	result = GOAHEAD;
	while (result == GOAHEAD) {
		switch(hsstate) {
		case HSST_WAIT_ATN:
			result = subsys_wait_atn();
			break;
		case HSST_WAIT_CMD:
			result = subsys_wait_cmd();
			break;
		case HSST_WAIT_DAV_RESET:
			result = subsys_wait_dav_reset();
			break;
		case HSST_STOPPED:
			if (cmdrecvd) {
				return GOAHEAD;
			}
			else {
				hsstate = HSST_WAIT_ATN;
			}
		}
	}
	return WAITING;
}


static int subsys_receive_data(void) {
	int result;

	result = GOAHEAD;
	while (result == GOAHEAD) {
		switch(hsstate) {
		case HSST_WAIT_DATA:
			result = subsys_wait_data();
			break;
		case HSST_WAIT_DAV_RESET:
			result = subsys_wait_dav_reset();
			break;
		case HSST_STOPPED:
			if (recvdatacnt > 0) {
				subsys_setportc(RFDBIT);
				hsstate = HSST_WAIT_DATA;
			}
			else {
				// �\�肵���o�C�g���̃f�[�^����M�ς�
				return GOAHEAD;
			}
		}
	}
	return WAITING;
}

static int subsys_send_data(void) {
	int result;

	result = GOAHEAD;
	while (result == GOAHEAD) {
		switch(hsstate) {
		case HSST_WAIT_RFD:
			result = subsys_wait_rfd();
			break;
		case HSST_WAIT_DAC:
			result = subsys_wait_dac();
			break;
		case HSST_WAIT_DAC_RESET:
			result = subsys_wait_dac_reset();
			break;
		case HSST_STOPPED:
			if (senddatacnt > 0) {
				hsstate = HSST_WAIT_RFD;
			}
			else {
				// �\�肵���o�C�g���𑗐M�ς�
				return GOAHEAD;
			}
		}
	}
	return WAITING;
}

/*
�R�}���h��M��A�p�����[�^��M�O�̏����B
��ɁA��M����p�����[�^�̃o�C�g�������肷��B
	�o��:
		recvdatacnt
		recvbuf
*/
static void subsys_cmd_received(void) {
	recvdatacnt = 0;
	switch (cmd) {
	case CMD_INITIALIZE:
	case CMD_SEND_DATA:
	case CMD_SEND_RESULT_STATUS:
	case CMD_SLEEP:
		break;
	case CMD_WRITE_DATA:
		recvdatacnt = 4;
		recvbuf = parambuf;
		parambuf[5] = 0;
		break;
	case CMD_READ_DATA:
		recvdatacnt = 4;
		recvbuf = parambuf;
		break;
	case CMD_RECEIVE_MEMORY:
		recvdatacnt = 4;
		recvbuf = parambuf;
		parambuf[5] = 0;
		break;
	case CMD_EXECUTE_COMMAND:
		recvdatacnt = 2;
		recvbuf = parambuf;
		break;
	case CMD_SET_DISK_MODE:
		recvdatacnt = 2;
		recvbuf = parambuf;
		break;
	case CMD_LOAD_DATA:
		recvdatacnt = 6;
		recvbuf = parambuf;
		break;
	case CMD_SET_SURFACE_MODE:
	case CMD_SET_BOUNDARY_MODE:
	case CMD_DRIVE_READY_CHECK:
	case CMD_SEND_DISK_MODE:
		recvdatacnt = 1;
		recvbuf = parambuf;
		break;
	case CMD_ACTIVE:
		recvdatacnt = 2;
		recvbuf = parambuf;
		break;
	default:
		break;
	}
}


/*
�f�B�X�N����̂��߂̃T�u���[�`��
*/

static void config_fdc_by_disk_mode(int drv, int track) {
	BYTE mode;

	mode = subsysmem[WORK_DISK_MODE + drv];

	fdc.rpm[drv] = 0;	// 1.44�ł͂Ȃ��B (ToDo: �ݒ���@�͐������H)
	switch ((mode >> 4) & 0x03) {
	case 0:	// 1D/2D
		// ToDo: 48TPI/96TPI�̐؂�ւ����ǂ�������??
		CTRL_FDMEDIA[drv] = DISKTYPE_2DD;
		break;
	case 1: // 1DD/2DD
		CTRL_FDMEDIA[drv] = DISKTYPE_2DD;
		break;
	case 2: // 1HD/2HD
		CTRL_FDMEDIA[drv] = DISKTYPE_2HD;
		break;
	}

	if ((mode & 0x38) == 0x28 && track == 0) {
		// 1HD/2HD��mode��SPC�r�b�g��1��track��0�̏ꍇ
		fdc.mf = 0x00;	// FM
	}
	else {
		fdc.mf = 0x40;	// MFM
	}

	fdc.N = mode & 0x03;	// �Z�N�^��	fdc.N��ݒ肷��΂悢�̂��H
	subsysmem[WORK_DM_N] = fdc.N;
	subsysmem[WORK_LAST_DISK_MODE] = mode;
}

static void set_command_status(BYTE status) {
	subsysmem[WORK_COMMAND_STATUS] = status;
	TRACEOUT(("fdsubsys: command_status=0x%02x", status));
}

/*
�e�R�}���h�̎��s
	�o��:
		senddatacnt
		sendbuf
*/

/*
0x00 ������
*/
static void subsys_exec_initialize(void) {
	TRACEOUT(("fdsubsys: initialize command"));
	/* �ȉ��̏ꍇ�A����VA����2D�œǂ߂��Ɗ��Ⴂ����V1/V2�Ɉڍs���悤�Ƃ���
	subsysmem[WORK_DISK_MODE + 0] = 0xff;
	subsysmem[WORK_DISK_MODE + 1] = 0xff;
	*/
	subsysmem[WORK_DISK_MODE + 0] = 0x01;
	subsysmem[WORK_DISK_MODE + 1] = 0x01;
}

/*
0x01 ���C�g�f�[�^
*/
static void subsys_exec_write_data(void) {
	if (parambuf[5] == 0) {
		BYTE sectorcnt = parambuf[0];
		BYTE drv = parambuf[1];
		BYTE track = parambuf[2];
		BYTE sector = parambuf[3];

		TRACEOUT(("fdsubsys: write data (not implemented): sectorcnt=%d, drv=%d, track=%d, sector=%d",sectorcnt, drv, track, sector));

		recvdatacnt = 256 * sectorcnt;
		if (recvdatacnt) {
			recvbuf = &subsysmem[WORK_DATA_BUF];
			state = ST_RECV_DATA;
			parambuf[5] = 1;		// �f�[�^�{�̂̎�M�ɂƂ肩���������Ƃ�\���t���O
		}
	}
	else {
	
	}
}

/*
0x02 �f�B�X�N����̓ǂݍ���
*/
static void subsys_exec_read_data(void) {
	int drv;
	int sectorcnt;
	int track;
	int sector;
	int readbufaddr;
	int sectorsize;

	sectorcnt = parambuf[0];
	drv = parambuf[1];
	track = parambuf[2];
	sector = parambuf[3];

	TRACEOUT(("fdsubsys: read_data: drv=%d, sectorcnt=%d, track=%d, sector=%d", drv, sectorcnt, track, sector));

	/*
	fdc
		us		�h���C�u�ԍ� (0�`)
		ctrlfd (CTRL_FDMEDIA) DISKTYPE_2HD or DISKTYPE_2DD
		treg	? ���݂̃V�����_�ԍ� �ݒ�s�v�H
		hd		�����w�b�h�ԍ�
		C,H,R,N
		mf		0:fm 0x40:mfm 0xff:??
		rpm		?	0:1.2  1:1.44

		ncn		�V�[�N��̃V�����_�ԍ�

	
	
	*/
	if (drv >= DRIVES) goto failed;

	fdc.us = drv;
	config_fdc_by_disk_mode(drv, track);

	fdc.ncn = track >> 1;
	fdc.hd = track & 1;
	if (fdd_seek()) goto failed;

	subsysmem[WORK_READ_SECTOR_COUNT] = 0;
	readbufaddr = WORK_DATA_BUF;
	while (sectorcnt > 0) {
		fdc.C = track >> 1;
		fdc.H = track & 1;
		fdc.R = sector;
		// fdc.N ��config_fdc_by_disk_mode�Őݒ�
		fdc.hd = track & 1;

		if (fdd_read()) goto failed;

		sectorsize = 128 << subsysmem[WORK_DM_N];
		CopyMemory(&subsysmem[readbufaddr], fdc.buf, sectorsize);

		sectorcnt--;
		sector++;
		subsysmem[WORK_READ_SECTOR_COUNT]++;
		readbufaddr += sectorsize;
		if (readbufaddr >= WORK_DATA_BUF + DATA_BUF_SIZE) goto failed;
	}
	set_command_status(0x40);
	return;

failed:
	set_command_status(0x01);
	return;
}

/*
0x03 ���[�h�o�b�t�@�ɂ���f�[�^���擾
*/
static void subsys_exec_send_data(void) {
	senddatacnt = (128 << subsysmem[WORK_DM_N]) * subsysmem[WORK_READ_SECTOR_COUNT];
	sendbuf = &subsysmem[WORK_DATA_BUF];
	TRACEOUT(("fdsubsys: send data: count=%d", senddatacnt));
}

/*
0x06 �R�}���h�X�e�[�^�X�̎擾
*/
static void subsys_exec_send_result_status(void) {
	parambuf[0] = subsysmem[WORK_COMMAND_STATUS];

	senddatacnt = 1;
	sendbuf = parambuf;

	TRACEOUT(("fdsubsys: send result status: return 0x%02x", parambuf[0]));

}

/*
0x0c �T�u�V�X�e�����ւ̃f�[�^�]��
*/
static void subsys_exec_receive_memory(void) {
	if (parambuf[5] == 0) {
		// �f�[�^�{�̂�����M
		WORD addr = (parambuf[0] << 8) | parambuf[1];
		recvdatacnt = (parambuf[2] << 8) | parambuf[3];
		recvbuf = &subsysmem[addr];
		state = ST_RECV_DATA;
		parambuf[5] = 1;		// �f�[�^�{�̂̎�M�ɂƂ肩���������Ƃ�\���t���O

		TRACEOUT(("fdsubsys: receive_memory: addr=0x%04x, bytes=%d", addr, recvdatacnt));
	}
	else {
		// �f�[�^�{�̂���M�ς�
	}

	/*
	VA2�̋N�����[�`���ł́A7f4bh(���[�h/���C�g�f�[�^�p���g���C�l)�Ƀf�[�^01h�܂���09h
	��]�����Ă���B�������Ă������x���Ȃ������B
	*/
}

/*
0x0d �G�O�[�L���[�g�E�R�}���h
*/
static void subsys_exec_execute_command(void) {
	WORD addr = (parambuf[0] << 8) | parambuf[1];

	TRACEOUT(("fdsubsys: execute command (not implemented): address=0x%02x",addr));
}

/*
0x0e ���[�h�E�f�[�^
*/
static void subsys_exec_load_data(void) {
	BYTE sectorcnt = parambuf[0];
	BYTE drv = parambuf[1];
	BYTE track = parambuf[2];
	BYTE sector = parambuf[3];
	WORD addr = (parambuf[4] << 8) | parambuf[5];

	TRACEOUT(("fdsubsys: load data (not implemented): sectorcount=%d, drv=%d, track=%d, sector=%d, address=0x%04x", sectorcnt, drv, track, sector, addr));
}


/*
0x17 �T�[�t�F�[�X���[�h�̐ݒ�
*/
static void subsys_exec_set_surface_mode(void) {
	TRACEOUT(("fdsubsys: set surface mode: mode=0x%02x",parambuf[0]));
}

/*
0x1f �f�B�X�N���[�h�̐ݒ�
*/
static void subsys_exec_set_disk_mode(void) {
	int drv;
	BYTE mode;

	drv = parambuf[0];
	mode = parambuf[1];

	TRACEOUT(("fdsubsys: set_disk_mode: drive=%d, mode=0x%02x", drv, mode));

	if (drv < DRIVES) {
		subsysmem[WORK_DISK_MODE + drv] = mode;
	}
}

/*
0x20 �f�B�X�N���[�h�̎擾
*/
static void subsys_exec_send_disk_mode(void) {
	int drv;
	BYTE mode = 0xff;

	drv = parambuf[0];


	if (drv < DRIVES) {
		mode = subsysmem[WORK_DISK_MODE + drv];
	}

	TRACEOUT(("fdsubsys: send_disk_mode: drive=%d, mode=0x%02x", drv, mode));

	parambuf[0] = mode;
	senddatacnt = 1;
	sendbuf = parambuf;
}

/*
0x21 �o�E���_�����[�h�̐ݒ�
*/
static void subsys_exec_set_boundary_mode(void) {
	TRACEOUT(("fdsubsys: set boundary mode: mode=%d", parambuf[0]));
}

/*
0x23 �h���C�u���f�B�`�F�b�N
*/
static void subsys_exec_drive_ready_check(void) {
	REG8 drv;

	drv = parambuf[0];

	if (fdd_diskready(drv)) {
		parambuf[0] = 0x00;
	}
	else {
		parambuf[0] = 0xff;		// �f�B�X�N���Z�b�g����Ă��Ȃ�
	}

	TRACEOUT(("fdsubsys: drive_ready_check: drive=%d, return=%d", drv, parambuf[0]));

	senddatacnt = 1;
	sendbuf = parambuf;
}

/*
0x25 �X���[�v
*/
static void subsys_exec_sleep(void) {
	TRACEOUT(("fdsubsys: sleep: return 0, 0"));
	parambuf[0] = 0;	// ���C���� I/O 1b2h (�T�u�V�X�e�� I/O f4h)�̒l
	parambuf[1] = 0;	// ���[�^�[��� 0x00.OFF, 0xff.ON

	senddatacnt = 2;
	sendbuf = parambuf;
}

/*
0x26 �A�N�e�B�u
*/
static void subsys_exec_active(void) {
	TRACEOUT(("fdsubsys: active: port1b2h=0x%0x, moter=%d", parambuf[0], parambuf[1]));
}

/*
	�o��:
		state
		senddatacnt
		sendbuf
*/
static void subsys_exec_cmd(void) {
	state = ST_SEND_DATA;
	senddatacnt = 0;

	switch (cmd) {
	case CMD_INITIALIZE:
		subsys_exec_initialize();
		break;
	case CMD_WRITE_DATA:
		subsys_exec_write_data();
		break;
	case CMD_READ_DATA:
		subsys_exec_read_data();
		break;
	case CMD_SEND_DATA:
		subsys_exec_send_data();
		break;
	case CMD_SEND_RESULT_STATUS:
		subsys_exec_send_result_status();
		break;
	case CMD_RECEIVE_MEMORY:
		subsys_exec_receive_memory();
		break;
	case CMD_EXECUTE_COMMAND:
		subsys_exec_execute_command();
		break;
	case CMD_LOAD_DATA:
		subsys_exec_load_data();
		break;
	case CMD_SET_SURFACE_MODE:
		subsys_exec_set_surface_mode();
		break;
	case CMD_SET_DISK_MODE:
		subsys_exec_set_disk_mode();
		break;
	case CMD_SEND_DISK_MODE:
		subsys_exec_send_disk_mode();
		break;
	case CMD_SET_BOUNDARY_MODE:
		subsys_exec_set_boundary_mode();
		break;
	case CMD_DRIVE_READY_CHECK:
		subsys_exec_drive_ready_check();
		break;
	case CMD_SLEEP:
		subsys_exec_sleep();
		break;
	case CMD_ACTIVE:
		subsys_exec_active();
		break;
	}
}

static void subsys_exec(void) {
	int result;

	result = GOAHEAD;
	while (result == GOAHEAD) {
		switch (state) {
		case ST_RECV_CMD:
			result = subsys_receive_cmd();
			if (result == GOAHEAD) {
				TRACEOUT(("fdsubsys: recv cmd 0x%02x", cmd));
				subsys_cmd_received();
				state = ST_RECV_DATA;
			}
			break;
		case ST_RECV_DATA:
			result = subsys_receive_data();
			if (result == GOAHEAD) {
				state = ST_EXEC_CMD;
			}
			break;
		case ST_EXEC_CMD:
			subsys_exec_cmd();
			break;
		case ST_SEND_DATA:
			result = subsys_send_data();
			if (result == GOAHEAD) {
				state = ST_END_CYCLE;
			}
			break;
		case ST_END_CYCLE:
			state = ST_RECV_CMD;
			cmdrecvd = FALSE;
			break;
		}
	}
}

static void subsys_reset(void) {
	state = ST_RECV_CMD;
	hsstate = HSST_STOPPED;
	cmdrecvd = FALSE;
	subsys_resetportc(DAVBIT);
	subsys_resetportc(RFDBIT);
	subsys_resetportc(DACBIT);
}

// ---- I/O

static void IOOUTCALL fdsubsys_o0fd(UINT port, REG8 dat) {
	portb_main = dat;
	subsys_exec();
	(void)port;
}

static void IOOUTCALL fdsubsys_o0fe(UINT port, REG8 dat) {
	portc_main = (portc_main & 0x0f) | (dat & 0xf0);
	subsys_exec();
	(void)port;
}

static void IOOUTCALL fdsubsys_o0ff(UINT port, REG8 dat) {
	if (dat & 0x80) {
		// 8255���[�h�w��
		// ����
	}
	else {
		int bitnum;

		bitnum = (dat >> 1) & 0x07;
		if (bitnum >= 4) {
			if (dat & 1) {
				// �Z�b�g
				portc_main |= ntobit[bitnum];
			}
			else {
				// ���Z�b�g
				portc_main &= ~ntobit[bitnum];
			}
			subsys_exec();
		}
	}
	(void)port;
}

static REG8 IOINPCALL fdsubsys_i0fc(UINT port) {
	(void)port;
	return porta_main;
}

static REG8 IOINPCALL fdsubsys_i0fe(UINT port) {
	(void)port;
	return portc_main;
}

// ---- I/F

void fdsubsys_reset(void) {
	subsys_reset();
	fdsubsys_o0fe(0, 0);
}

void fdsubsys_bind(void) {
	iocoreva_attachout(0x0fd, fdsubsys_o0fd);
	iocoreva_attachout(0x0fe, fdsubsys_o0fe);
	iocoreva_attachout(0x0ff, fdsubsys_o0ff);

	iocoreva_attachinp(0x0fc, fdsubsys_i0fc);
	iocoreva_attachinp(0x0fe, fdsubsys_i0fe);
}

#endif
