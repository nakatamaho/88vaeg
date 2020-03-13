#if defined(SUPPORT_PC88VA)

typedef struct {
	BYTE	*dst;
	int		width;					// ��ʕ�(pixel)
	int		xbytes;					// ��ʕ�(byte)
	int		y;
	int		xalign;					// 1�s�N�Z���̃o�C�g��
	int		yalign;					// 1���X�^�̃o�C�g��
	BYTE	dirty[SURFACE_WIDTH];
} _SDRAWVA, *SDRAWVA;

typedef void (SCRNCALL * SDRAWFNVA)(SDRAWVA sdraw, int maxy);


#ifdef __cplusplus
extern "C" {
#endif

const SDRAWFNVA sdrawva_getproctbl(const SCRNSURF *surf);

#ifdef __cplusplus
}
#endif

#endif