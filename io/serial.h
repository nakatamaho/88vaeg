
enum {
	KB_CTR			= (1 << 3),
	KB_CTRMASK		= (KB_CTR - 1),

	KB_BUF			= (1 << 7),
	KB_BUFMASK		= (KB_BUF - 1),

#if defined(SUPPORT_PC88VA)
	KB_MAP			= 0x10,
#endif

};

typedef struct {
	UINT32	xferclock;
	UINT8	data;
	UINT8	cmd;
	UINT8	mode;
	UINT8	status;
	UINT	ctrls;
	UINT	ctrpos;
	UINT	buffers;
	UINT	bufpos;
	UINT8	ctr[KB_CTR];
	UINT8	buf[KB_BUF];

#if defined(SUPPORT_PC88VA)
	UINT8	keymap[KB_MAP];
#endif

} _KEYBRD, *KEYBRD;

typedef struct {
	UINT8	result;
	UINT8	data;
	UINT8	send;
#if defined(VAEG_FIX)
	UINT8	cmd;
#else
	UINT8	pad;
#endif
	UINT	pos;
	UINT	dummyinst;
	UINT	mul;
} _RS232C, *RS232C;



#ifdef __cplusplus
extern "C" {
#endif

void keyboard_callback(NEVENTITEM item);

void keyboard_reset(void);
void keyboard_bind(void);
void keyboard_resetsignal(void);
void keyboard_ctrl(REG8 data);
void keyboard_send(REG8 data);



void rs232c_construct(void);
void rs232c_destruct(void);

void rs232c_reset(void);
void rs232c_bind(void);
void rs232c_open(void);
void rs232c_callback(void);

BYTE rs232c_stat(void);
void rs232c_midipanic(void);

#ifdef __cplusplus
}
#endif

