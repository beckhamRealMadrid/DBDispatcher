#include "pch.h"
#include "CAccessLock.h"

CAccessLock::CAccessLock() : __mFlag(false)
{

}

// ���� ȹ�� (�����ϸ� true, �����ϸ� false ��ȯ)
bool CAccessLock::Acquire()
{
	bool aExpected = false;
	if (__mFlag.compare_exchange_strong(aExpected, true, std::memory_order_acquire))
	{
#ifdef _DEBUG
		__mOwnerThreadID = GetCurrentThreadId();
#endif
		return true;
	}

	return false;
}

// ���� ���� (ȹ���� �����常 ȣ�� ����)
VOID CAccessLock::Release()
{
#ifdef _DEBUG
	if (GetCurrentThreadId() != __mOwnerThreadID)
	{
		throw std::runtime_error("Unauthorized thread attempted to release the lock.");
	}
#endif
	__mFlag.store(false, std::memory_order_release);
#ifdef _DEBUG
	__mOwnerThreadID = 0;
#endif
}

// �ʿ� �� ���ܸ� ������ Acquire �Լ� (������ ���)
VOID CAccessLock::AcquireOrThrow()
{
	if (!Acquire())
	{
		throw std::runtime_error("Failed to acquire the lock.");
	}
}

// ���� ������ �ʱ�ȭ (���� ����)
VOID CAccessLock::ResetFlag()
{
	__mFlag.store(false, std::memory_order_relaxed);
#ifdef _DEBUG
	__mOwnerThreadID = 0;
#endif
}

// ���� ������ ���� ������ ID ��ȯ (����� �뵵)
#ifdef _DEBUG
DWORD CAccessLock::GetOwnerThreadID() const
{
	return __mOwnerThreadID;
}
#endif