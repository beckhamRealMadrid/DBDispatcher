#include "pch.h"
#include "CAccessLock.h"

CAccessLock::CAccessLock() : __mFlag(false)
{

}

// 권한 획득 (성공하면 true, 실패하면 false 반환)
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

// 권한 해제 (획득한 스레드만 호출 가능)
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

// 필요 시 예외를 던지는 Acquire 함수 (선택적 사용)
VOID CAccessLock::AcquireOrThrow()
{
	if (!Acquire())
	{
		throw std::runtime_error("Failed to acquire the lock.");
	}
}

// 권한 정보를 초기화 (강제 리셋)
VOID CAccessLock::ResetFlag()
{
	__mFlag.store(false, std::memory_order_relaxed);
#ifdef _DEBUG
	__mOwnerThreadID = 0;
#endif
}

// 현재 권한을 가진 스레드 ID 반환 (디버깅 용도)
#ifdef _DEBUG
DWORD CAccessLock::GetOwnerThreadID() const
{
	return __mOwnerThreadID;
}
#endif