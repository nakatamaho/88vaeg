// ----------------------------------------------------------------------------
//	M88 - PC-8801 series emulator
//	Copyright (C) cisc 1999.
// ----------------------------------------------------------------------------
//	$Id: types.h,v 1.2 2006/04/10 05:07:55 shinra Exp $

#define VAEG		// Shinra

#if !defined(win32_types_h)
#define win32_types_h

#define ENDIAN_IS_SMALL

//  固定長型とか
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

// 8 bit 数値をまとめて処理するときに使う型
typedef uint32 packed;
#define PACK(p) ((p) | ((p) << 8) | ((p) << 16) | ((p) << 24))

// ボインタ値を表現できる整数型
typedef uint intpointer;

// 関数へのポインタにおいて 常に 0 となるビット (1 bit のみ)
// なければ PTR_IDBIT 自体を define しないでください．
// (x86 版 Z80 エンジンでは必須)

#if defined(_DEBUG)
	#define PTR_IDBIT	0x80000000
#else
	#define PTR_IDBIT	0x1
#endif

// ワード境界を越えるアクセスを許可
#define ALLOWBOUNDARYACCESS

#if !defined(VAEG)
// x86 版の Z80 エンジンを使用する
#define USE_Z80_X86
#endif

// C++ の新しいキャストを使用する(但し win32 コードでは関係なく使用する)
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
