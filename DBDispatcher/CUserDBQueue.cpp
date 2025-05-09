#include "pch.h"
#include "CUserDBQueue.h"

CUserDBQueue::CUserDBQueue()
{
	__ResetAttr();
}

CUserDBQueue::~CUserDBQueue()
{
	__Dtor();
}

VOID CUserDBQueue::__ResetAttr()
{
	__mAccessLock.ResetFlag();
	__mDBMsgQueue.clear();
}

VOID CUserDBQueue::__Dtor()
{
	
}

DWORD CUserDBQueue::Open(DWORD pCapacity, WORD pAccount)
{
	DWORD aRv = 0;

	aRv = __mLocker.Open(FALSE);
	if (0 < aRv)
	{
		return aRv;
	}

	aRv = __mDBMsgQueue.Open(pCapacity);
	if (0 < aRv)
	{
		return aRv;
	}

	__mAccount = pAccount;

	return 0;
}

VOID CUserDBQueue::Close()
{
	__mDBMsgQueue.Close();
	__mLocker.Close();
}

VOID CUserDBQueue::Reset()
{
	__ResetAttr();
}

BOOL CUserDBQueue::LockAccess()
{
	return __mAccessLock.Acquire();
}

VOID CUserDBQueue::UnlockAccess()
{
	__mAccessLock.Release();
}

DWORD CUserDBQueue::Enqueue(WORD pCommand, const void* pPayload, DWORD pSize)
{
	__TLockerAuto aLocker(__mLocker);
	if (__mDBMsgQueue.Full())
	{
		return ERROR_DESTINATION_ELEMENT_FULL;
	}

	CDBMsg aMsg(pCommand, pPayload, pSize);
	__mDBMsgQueue.push_back(std::move(aMsg));
	
	return 0;
}

BOOL CUserDBQueue::Dequeue(CDBMsg* pMsg)
{
	assert(GetCurrentThreadId() == __mAccessLock.GetOwnerThreadID());

	__TLockerAuto aLocker(__mLocker);
	if (__mDBMsgQueue.empty())
	{
		return FALSE;
	}

	*pMsg = std::move(__mDBMsgQueue.front());
	__mDBMsgQueue.pop_front();
	
	return TRUE;
}

VOID CUserDBQueue::PopBack(VOID)
{
	__TLockerAuto aLocker(__mLocker);
	__mDBMsgQueue.pop_back();
}

WORD CUserDBQueue::GetAccount() const
{ 
	return __mAccount;
}