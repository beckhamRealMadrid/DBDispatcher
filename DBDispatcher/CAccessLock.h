#pragma once

#include <atomic>

/**
 * 단일 스레드만 접근할 수 있도록 atomic 플래그로 제어하는 락 클래스이다.
 */
class CAccessLock
{
public:
	CAccessLock();
	bool	Acquire();
	VOID	Release();
	VOID	AcquireOrThrow();
	VOID	ResetFlag();
#ifdef _DEBUG
	DWORD	GetOwnerThreadID() const;
#endif
private:
	std::atomic<bool>	__mFlag;  // 권한 플래그 (원자적 접근)
#ifdef _DEBUG
	DWORD	__mOwnerThreadID = 0; // 디버그 모드에서만 활성화
#endif
};