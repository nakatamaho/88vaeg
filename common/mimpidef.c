#include	"compiler.h"
#include	"dosio.h"
#include	"textfile.h"
#include	"mimpidef.h"


static const char str_la[] = "LA";
static const char str_pcm[] = "PCM";
static const char str_gs[] = "GS";
static const char str_rhythm[] = "RHYTHM";


static char *cutdelimit(char *buf) {

	while((buf[0] > 0) && (buf[0] <= ' ')) {
		buf++;
	}
	return(buf);
}

static BOOL getvalue(char **buf, int *value) {

	char	*p;
	int		val;
	BOOL	ret;
	char	c;

	p = cutdelimit(*buf);
	val = 0;
	ret = FAILURE;
	while(1) {
		c = *p++;
		c -= '0';
		if ((c & 0xff) >= 10) {
			break;
		}
		val *= 10;
		val += c;
		ret = SUCCESS;
	}
	if (ret == SUCCESS) {
		*buf = p;
		*value = val;
	}
	return(ret);
}

static void analyze(MIMPIDEF *def, char *buf) {

	int		num;
	int		mod;
	int		bank;
	int		tone;

	buf = cutdelimit(buf);
	if (buf[0] == '@') {
		buf++;
		if ((getvalue(&buf, &num) != SUCCESS) || (num < 1) || (num > 16)) {
			return;
		}
		num--;
		buf = cutdelimit(buf);
		if (!milstr_memcmp(buf, str_la)) {
			def->ch[num] = MIMPI_LA;
		}
		else if (!milstr_memcmp(buf, str_pcm)) {
			def->ch[num] = MIMPI_PCM;
		}
		else if (!milstr_memcmp(buf, str_gs)) {
			def->ch[num] = MIMPI_GS;
		}
		else if (!milstr_memcmp(buf, str_rhythm)) {
			def->ch[num] = MIMPI_RHYTHM;
		}
	}
	else {
		if ((getvalue(&buf, &mod) != SUCCESS) ||
			(mod < 0) || (mod >= MIMPI_RHYTHM)) {
			return;
		}
		if ((getvalue(&buf, &num) != SUCCESS) || (num < 1) || (num > 128)) {
			return;
		}
		if ((getvalue(&buf, &tone) != SUCCESS) ||
			(tone < 1) || (tone > 128)) {
			return;
		}
		num--;
		tone--;
		if (buf[0] == ':') {
			buf++;
			bank = tone;
			if ((getvalue(&buf, &tone) != SUCCESS) ||
				(tone < 1) || (tone > 128)) {
				return;
			}
			tone--;
			def->bank[mod][num] = (BYTE)bank;
		}
		def->map[mod][num] = (BYTE)tone;
	}
}

BOOL mimpidef_load(MIMPIDEF *def, const char *filename) {

	BYTE		b;
	TEXTFILEH	fh;
	char		buf[256];

	if (def == NULL) {
		goto mdld_err;
	}
	ZeroMemory(def->ch, sizeof(def->ch));
	def->ch[9] = MIMPI_RHYTHM;
	for (b=0; b<128; b++) {
		def->map[0][b] = b;
		def->map[1][b] = b;
		def->map[2][b] = b;
	}
	if ((filename == NULL) || (!filename[0])) {
		goto mdld_err;
	}
	fh = textfile_open(filename, 512);
	if (fh == NULL) {
		goto mdld_err;
	}
	while(textfile_read(fh, buf, sizeof(buf)) == SUCCESS) {
		analyze(def, buf);
	}
	textfile_close(fh);
	return(SUCCESS);

mdld_err:
	return(FAILURE);
}

