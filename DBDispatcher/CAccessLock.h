#pragma once

#include <atomic>

/**
 * ���� �����常 ������ �� �ֵ��� atomic �÷��׷� �����ϴ� �� Ŭ�����̴�.
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
	std::atomic<bool>	__mFlag;  // ���� �÷��� (������ ����)
#ifdef _DEBUG
	DWORD	__mOwnerThreadID = 0; // ����� ��忡���� Ȱ��ȭ
#endif
};