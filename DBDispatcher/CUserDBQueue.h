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
	__TLockerAuto::TLocker	__mLocker;		// ������ ����ȭ ��ü
	CAccessLock				__mAccessLock;	// ���� ������ ���� ������ ���� ���� ���� ��
	CDBMsgQueue				__mDBMsgQueue;	// ���� ���� DB ��û�� �����ϴ� ���� ũ�� ��ȯ ť
	WORD					__mAccount;
};