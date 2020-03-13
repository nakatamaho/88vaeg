// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//	$Id: types.h,v 1.2 2006/04/10 05:07:55 shinra Exp $

#define VAEG		// Shinra

#if !defined(win32_types_h)
#define win32_types_h

#define ENDIAN_IS_SMALL

//  �Œ蒷�^�Ƃ�
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;

typedef signed char sint8;
typedef signed short sint16;
typedef signed int sint32;

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;

// 8 bit ���l���܂Ƃ߂ď�������Ƃ��Ɏg���^
typedef uint32 packed;
#define PACK(p) ((p) | ((p) << 8) | ((p) << 16) | ((p) << 24))

// �{�C���^�l��\���ł��鐮���^
typedef uint intpointer;

// �֐��ւ̃|�C���^�ɂ����� ��� 0 �ƂȂ�r�b�g (1 bit �̂�)
// �Ȃ���� PTR_IDBIT ���̂� define ���Ȃ��ł��������D
// (x86 �� Z80 �G���W���ł͕K�{)

#if defined(_DEBUG)
	#define PTR_IDBIT	0x80000000
#else
	#define PTR_IDBIT	0x1
#endif

// ���[�h���E���z����A�N�Z�X������
#define ALLOWBOUNDARYACCESS

#if !defined(VAEG)
// x86 �ł� Z80 �G���W�����g�p����
#define USE_Z80_X86
#endif

// C++ �̐V�����L���X�g���g�p����(�A�� win32 �R�[�h�ł͊֌W�Ȃ��g�p����)
#define USE_NEW_CAST

// ---------------------------------------------------------------------------

#if !defined(VAEG)
#ifdef USE_Z80_X86
	#define MEMCALL __stdcall
#else
	#define MEMCALL
#endif
#endif

#if defined(USE_NEW_CAST) && defined(__cplusplus) 
	#define STATIC_CAST(t, o)			static_cast<t> (o)
	#define REINTERPRET_CAST(t, o)		reinterpret_cast<t> (o)
#else
	#define STATIC_CAST(t, o)			((t)(o))
	#define REINTERPRET_CAST(t, o)		(*(t*)(void*)&(o))
#endif

#endif // win32_types_h
