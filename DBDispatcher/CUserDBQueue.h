#pragma once

#include <windows.h>
#include "CAccessLock.h"
#include "CDBMsgQueue.h"
#include "CLocker.h"
#include "CDBMsg.h"

class CUserDBQueue
{
public:
	CUserDBQueue();
	~CUserDBQueue();
	DWORD	Open(DWORD pCapacity, WORD pAccount);
	VOID	Close();
	VOID	Reset();
	BOOL	LockAccess();
	VOID	UnlockAccess();
	DWORD	Enqueue(WORD pCommand, const void* pPayload, DWORD pSize);
	VOID	PopBack(VOID);
	BOOL	Dequeue(CDBMsg* pMsg);
	WORD	GetAccount() const;
private:
	VOID	__ResetAttr();
	VOID	__Dtor();
private:
	typedef CLockerAuto<CLocker>	__TLockerAuto;
private:
	__TLockerAuto::TLocker	__mLocker;		// 스레드 동기화 객체
	CAccessLock				__mAccessLock;	// 단일 스레드 접근 보장을 위한 권한 제어 락
	CDBMsgQueue				__mDBMsgQueue;	// 유저 단위 DB 요청을 보관하는 고정 크기 순환 큐
	WORD					__mAccount;
};