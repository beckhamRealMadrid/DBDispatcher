#include "pch.h"
#include "CDBMsgPool.h"

CDBMsgPool::CDBMsgPool()
{
	__ResetAttr();
}

CDBMsgPool::~CDBMsgPool()
{
	__Dtor();
}

VOID CDBMsgPool::__ResetAttr()
{
	__mIsOpen = FALSE;
}

VOID CDBMsgPool::__Dtor()
{
	if (__mAllMsg.IsOpen())
	{
		for (DWORD i = 0; i < __mAllMsg.GetSize(); ++i)
		{
			CDBMsg* aMsg = __mAllMsg[i];
			if (aMsg != nullptr)
			{
				delete aMsg;
				__mAllMsg[i] = nullptr;
			}
		}
		__mAllMsg.Close();
	}

	__mFreeSemaphoreChannel.Close();
	__ResetAttr();
}

DWORD CDBMsgPool::Open(DWORD pCount)
{
	if (__mIsOpen || pCount == 0)
		return ERROR_INVALID_PARAMETER;

	DWORD aRv = 0;

	aRv = __mAllMsg.Open(pCount);
	if (aRv != 0)
	{
		return aRv;
	}

	aRv = __mFreeSemaphoreChannel.Open();
	if (aRv != 0)
	{
		return aRv;
	}

	for (DWORD i = 0; i < pCount; ++i)
	{
		CDBMsg* aMsg = new CDBMsg;
		if (aMsg == nullptr)
			return ERROR_NOT_ENOUGH_MEMORY;

		__mAllMsg[i] = aMsg;

		aRv = __mFreeSemaphoreChannel.PushAndSignal(aMsg);
		if (aRv != 0)
			return aRv;
	}

	__mIsOpen = TRUE;
	return 0;
}

VOID CDBMsgPool::Close()
{
	__Dtor();
}

BOOL CDBMsgPool::IsOpen() const
{
	return __mIsOpen;
}

CDBMsg* CDBMsgPool::Get(DWORD pTimeout)
{
	assert(IsOpen());

	CDBMsg* aMsg = nullptr;
	DWORD aRv = __mFreeSemaphoreChannel.WaitAndPop(aMsg, pTimeout);
	if (aRv != 0 || aMsg == nullptr)
	{
		printf("[CDBMsgPool::Get] Pool exhausted or semaphore error (rv=%u, ptr=%p)", aRv, aMsg);
	}
	
	return aMsg;
}

DWORD CDBMsgPool::Release(CDBMsg* pMsg)
{
	assert(IsOpen());

	return __mFreeSemaphoreChannel.PushAndSignal(pMsg);
}

DWORD CDBMsgPool::GetSize() const
{
	assert(IsOpen());

	return __mAllMsg.GetSize();
}

// 풀에서 현재 재사용 가능한 CDBMsg 객체의 개수를 반환
DWORD CDBMsgPool::GetFreeCount() const
{
	assert(IsOpen());

	return __mFreeSemaphoreChannel.GetSz();
}

BOOL CDBMsgPool::IsMember(const CDBMsg* pMsg) const
{
	assert(IsOpen());

	for (DWORD i = 0; i < __mAllMsg.GetSize(); ++i)
	{
		if (__mAllMsg[i] == pMsg)
			return TRUE;
	}

	return FALSE;
}