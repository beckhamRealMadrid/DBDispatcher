#pragma once

#include <deque>
#include "CLocker.h"

class CDBMsg;

/**
 * ������� ����� ������ �� �޽����� �����ϰ� �ۼ����ϴ� ����ȭ ť Ŭ�����̴�.

   WaitForSingleObjectEx, ReleaseSemaphore ����ȭ ����
   1. ReleaseSemaphore() : ī��Ʈ�� ���� (��ȣ �߻�)
   2. WaitForSingleObjectEx() : ī��Ʈ�� �����ϸ鼭 ��ȣ ���

   Scenario 1: WaitForSingleObjectEx()�� ���� ��� ��
   1. �����尡 WaitForSingleObjectEx(__mSemaphore, INFINITE, TRUE)�� ���
   2. ���� �������� ī��Ʈ�� 0�̶� ���
   3. ������ ReleaseSemaphore(__mSemaphore, 1, NULL) ȣ��
   4. ��� ���� �����尡 ��� ���
   5. �������� ī��Ʈ�� �ٽ� 0�� ��

   �ʱ� ����: [�������� ī��Ʈ: 0]
   Thread A: WaitForSingleObjectEx(...)  -> ���
   Thread B: ReleaseSemaphore(..., 1)    -> Thread A ���
      
   Scenario 2: ReleaseSemaphore()�� ���� �����
   1. ReleaseSemaphore(__mSemaphore, 1, NULL) ȣ�� -> �������� ī��Ʈ�� 1�� ��
   2. ���� �ƹ��� WaitForSingleObjectEx() ȣ�� �� ��
   3. ���߿� �����尡 WaitForSingleObjectEx(...) ȣ��
   4. �������� ī��Ʈ�� 1�̹Ƿ� ��� ��ȯ�� (��� ����)
   5. ī��Ʈ�� �ٽ� 0���� ����

   �ʱ� ����: [�������� ī��Ʈ: 0]
   Thread B: ReleaseSemaphore(..., 1)    -> [ī��Ʈ = 1]
   Thread A: WaitForSingleObjectEx(...)  -> ��� ��ȯ
 */
class CSemaphoreChannel
{
public:
	CSemaphoreChannel();
	~CSemaphoreChannel();
public:
	DWORD	Open();
	VOID	Close();
	BOOL	IsOpen() const;
	VOID	Clear();
	DWORD	PushAndSignal(CDBMsg* pMsg);
	DWORD	WaitAndPop(CDBMsg*& pMsg, DWORD pTimeout = INFINITE);
	DWORD	GetSz() const;
	BOOL	IsEmpty() const;
	BOOL	IsExist(CDBMsg* pMsg) const;
	bool	Pop(CDBMsg*& pMsg);
private:
	VOID	__ResetAttr();
	VOID	__Dtor();
private:
	typedef CLockerAuto<CLocker>	__TLockerAuto;
private:
	__TLockerAuto::TLocker	__mLocker;		// ������ ����ȭ ��ü
	HANDLE					__mSemaphore;	// �������� �ڵ�
	std::deque<CDBMsg*>		__mQueue;		// �۾��� �����ϴ� ť
};