
#ifdef __cplusplus
extern "C" {
#endif

// １文字分のサイズを取得
int milank_charsize(const OEMCHAR *str);
int milsjis_charsize(const char *str);
int mileuc_charsize(const char *str);
int milutf8_charsize(const char *str);

// 大文字小文字を同一視して比較
// ret 0:一致
int milank_cmp(const OEMCHAR *str, const OEMCHAR *cmp);
int milsjis_cmp(const char *str, const char *cmp);
int mileuc_cmp(const char *str, const char *cmp);
int milutf8_cmp(const char *str, const char *cmp);

// 大文字小文字を 同一視してcmpのヌルまで比較
// ret 0:一致
int milank_memcmp(const OEMCHAR *str, const OEMCHAR *cmp);
int milsjis_memcmp(const char *str, const char *cmp);
int mileuc_memcmp(const char *str, const char *cmp);
int milutf8_memcmp(const char *str, const char *cmp);

// str[pos]が漢字１バイト目かどうか…
int milsjis_kanji1st(const char *str, int pos);
int mileuc_kanji1st(const char *str, int pos);
int milutf8_kanji1st(const char *str, int pos);

// str[pos]が漢字２バイト目かどうか…
int milsjis_kanji2nd(const char *str, int pos);
int mileuc_kanji2nd(const char *str, int pos);
int milutf8_kanji2nd(const char *str, int pos);

// maxlen分だけ文字列をコピー
void milank_ncpy(OEMCHAR *dst, const OEMCHAR *src, int maxlen);
void milsjis_ncpy(char *dst, const char *src, int maxlen);
void mileuc_ncpy(char *dst, const char *src, int maxlen);
void milutf8_ncpy(char *dst, const char *src, int maxlen);

// maxlen分だけ文字列をキャット
void milank_ncat(OEMCHAR *dst, const OEMCHAR *src, int maxlen);
void milsjis_ncat(char *dst, const char *src, int maxlen);
void mileuc_ncat(char *dst, const char *src, int maxlen);
void milutf8_ncat(char *dst, const char *src, int maxlen);

// 文字を検索
char *milank_chr(const OEMCHAR *str, int c);
char *milsjis_chr(const char *str, int c);
char *mileuc_chr(const char *str, int c);
char *milutf8_chr(const char *str, int c);


// 0~9, A~Z のみを大文字小文字を同一視して比較
// ret 0:一致
int milstr_extendcmp(const OEMCHAR *str, const OEMCHAR *cmp);

// 次の語を取得
char *milstr_nextword(const OEMCHAR *str);

// 文字列からARGの取得
int milstr_getarg(OEMCHAR *str, OEMCHAR *arg[], int maxarg);

// HEX2INT
long milstr_solveHEX(const OEMCHAR *str);

// STR2INT
long milstr_solveINT(const OEMCHAR *str);

// STRLIST
OEMCHAR *milstr_list(const OEMCHAR *lststr, UINT pos);

#ifdef __cplusplus
}
#endif


// ---- macros

#if defined(OSLANG_SJIS)
#define milstr_charsize(s)		milsjis_charsize(s)
#define	milstr_cmp(s, c)		milsjis_cmp(s, c)
#define	milstr_memcmp(s, c)		milsjis_memcmp(s, c)
#define	milstr_kanji1st(s, p)	milsjis_kanji1st(s, p)
#define	milstr_kanji2nd(s, p)	milsjis_kanji2nd(s, p)
#define	milstr_ncpy(d, s, l)	milsjis_ncpy(d, s, l)
#define	milstr_ncat(d, s, l)	milsjis_ncat(d, s, l)
#define	milstr_chr(s, c)		milsjis_chr(s, c)
#elif defined(OSLANG_EUC)
#define milstr_charsize(s)		mileuc_charsize(s)
#define	milstr_cmp(s, c)		mileuc_cmp(s, c)
#define	milstr_memcmp(s, c)		mileuc_memcmp(s, c)
#define	milstr_kanji1st(s, p)	mileuc_kanji1st(s, p)
#define	milstr_kanji2nd(s, p)	mileuc_kanji2nd(s, p)
#define	milstr_ncpy(d, s, l)	mileuc_ncpy(d, s, l)
#define	milstr_ncat(d, s, l)	mileuc_ncat(d, s, l)
#define	milstr_chr(s, c)		mileuc_chr(s, c)
#elif defined(OSLANG_UTF8)
#define milstr_charsize(s)		milutf8_charsize(s)
#define	milstr_cmp(s, c)		milutf8_cmp(s, c)
#define	milstr_memcmp(s, c)		milutf8_memcmp(s, c)
#define	milstr_kanji1st(s, p)	milutf8_kanji1st(s, p)
#define	milstr_kanji2nd(s, p)	milutf8_kanji2nd(s, p)
#define	milstr_ncpy(d, s, l)	milutf8_ncpy(d, s, l)
#define	milstr_ncat(d, s, l)	milutf8_ncat(d, s, l)
#define	milstr_chr(s, c)		milutf8_chr(s, c)
#else
#define milstr_charsize(s)		milank_charsize(s)
#define	milstr_cmp(s, c)		milank_cmp(s, c)
#define	milstr_memcmp(s, c)		milank_memcmp(s, c)
#define	milstr_kanji1st(s, p)	(0)
#define	milstr_kanji2nd(s, p)	(0)
#define	milstr_ncpy(d, s, l)	milank_ncpy(d, s, l)
#define	milstr_ncat(d, s, l)	milank_ncat(d, s, l)
#define	milstr_chr(s, c)		milank_chr(s, c)
#endif

