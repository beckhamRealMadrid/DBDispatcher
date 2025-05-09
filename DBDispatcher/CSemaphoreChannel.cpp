#include "pch.h"
#include "CSemaphoreChannel.h"

CSemaphoreChannel::CSemaphoreChannel()
{
	__ResetAttr();
}

CSemaphoreChannel::~CSemaphoreChannel()
{
	__Dtor();
}

VOID CSemaphoreChannel::__ResetAttr()
{
	__mSemaphore = NULL;
}

VOID CSemaphoreChannel::__Dtor()
{
	if (NULL != __mSemaphore)
	{
		CloseHandle(__mSemaphore);
		__mSemaphore = NULL;
	}

	__mQueue.clear();
}

DWORD CSemaphoreChannel::Open()
{
	assert(!IsOpen());

	DWORD aRv = 0;

	aRv = __mLocker.Open(TRUE);
	if (0 < aRv)
	{
		return aRv;
	}

	__mSemaphore = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
	if (NULL == __mSemaphore)
	{
		return GetLastError();
	}

	return 0;
}

VOID CSemaphoreChannel::Close()
{
	__Dtor();
	__mLocker.Close();
	__ResetAttr();
}

BOOL CSemaphoreChannel::IsOpen() const
{
	return __mLocker.IsOpen();
}

VOID CSemaphoreChannel::Clear()
{
	assert(IsOpen());

	__TLockerAuto aLocker(__mLocker);
	__mQueue.clear();
}

// ť�� �߰��ϰ� �������� ��ȣ ����
DWORD CSemaphoreChannel::PushAndSignal(CDBMsg* pMsg)
{
	assert(IsOpen());
	assert(NULL != __mSemaphore);

	DWORD aRv = 0;
	{
		__TLockerAuto aLocker(__mLocker);
		__mQueue.push_back(pMsg);
	}

	if (!ReleaseSemaphore(__mSemaphore, 1, NULL)) // ���������� ī��Ʈ�� �������� ��� ���� �����带 �����.
	{
		aRv = GetLastError();
		printf("[CSemaphoreChannel::PushAndSignal] Failed to release semaphore - Error code: %u", aRv);
	}

	return aRv;
}

// �������� ��ȣ�� ��ٸ� �� �۾��� ������
DWORD CSemaphoreChannel::WaitAndPop(CDBMsg*& pMsg, DWORD pTimeout)
{
	assert(IsOpen());
	assert(NULL != __mSemaphore);

	pMsg = nullptr;

	DWORD aRv = WaitForSingleObjectEx(__mSemaphore, pTimeout, TRUE);
	switch (aRv)
	{
		case WAIT_OBJECT_0: // �������� ��ȣ ����
		{
			__TLockerAuto aLocker(__mLocker);
			if (!__mQueue.empty())
			{
				pMsg = __mQueue.front();
				__mQueue.pop_front();
				return 0; // ����
			}
			return ERROR_EMPTY;
		}
		case WAIT_TIMEOUT:
			return WAIT_TIMEOUT;

		case WAIT_IO_COMPLETION:
			// alertable wait �� APC �Ϸ�� ��� -> Ư���� ó�� �� �ϰ� ������ ����
			return WAIT_IO_COMPLETION;

		case WAIT_FAILED:
		default:
			DWORD aErr = GetLastError();
			printf("CSemaphoreChannel::Receive() - Wait failed! Error: %u\n", aErr);
			return aErr;
	}
}

bool CSemaphoreChannel::Pop(CDBMsg*& pMsg)
{
	assert(IsOpen());

	__TLockerAuto lock(__mLocker);
	if (__mQueue.empty())
	{
		pMsg = nullptr;
		return false;
	}

	pMsg = __mQueue.front();
	__mQueue.pop_front();
	
	return true;
}

DWORD CSemaphoreChannel::GetSz() const
{
	assert(IsOpen());

	__TLockerAuto aLocker(__mLocker);
	return static_cast<DWORD>(__mQueue.size());
}

BOOL CSemaphoreChannel::IsEmpty() const
{
	assert(IsOpen());

	__TLockerAuto aLocker(__mLocker);
	return __mQueue.empty();
}

BOOL CSemaphoreChannel::IsExist(CDBMsg* pMsg) const
{
	assert(IsOpen());

	__TLockerAuto aLocker(__mLocker);
	return std::find(__mQueue.begin(), __mQueue.end(), pMsg) != __mQueue.end();
}