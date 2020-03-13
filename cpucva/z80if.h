/*
 * Z80if.h: Z80 interfaces
 */

#if !defined(__Z80if_h__)
#define __Z80if_h__

#include "types.h"

#ifndef IFCALL
#define IFCALL __stdcall
#endif
#ifndef IOCALL
#define IOCALL __stdcall
#endif


// ----------------------------------------------------------------------------
//	��������ԂɃA�N�Z�X���邽�߂̃C���^�[�t�F�[�X
//
struct IMemoryAccess
{
	virtual uint IFCALL	Read8(uint addr) = 0;
	virtual void IFCALL Write8(uint addr, uint data) = 0;
};

// ----------------------------------------------------------------------------
//	IO ��ԂɃA�N�Z�X���邽�߂̃C���^�[�t�F�[�X
//
struct IIOAccess
{
	virtual uint IFCALL In(uint port) = 0;
	virtual void IFCALL Out(uint port, uint data) = 0;
};


// ----------------------------------------------------------------------------
//  ���ݎ���(�N���b�N)�ɃA�N�Z�X���邽�߂̃C���^�t�F�[�X
//

struct IClock
{
	virtual uint32 IFCALL now() = 0;
};

// ----------------------------------------------------------------------------
//  �o�ߎ��ԋL�^�C���^�t�F�[�X
//
struct IClockCounter
{
	virtual void IFCALL past(sint32 clock) = 0;
	virtual sint32 IFCALL GetRemainclock() = 0;
	virtual void IFCALL SetRemainclock(sint32 clock) = 0;
};

#endif // __Z80_if_h__
