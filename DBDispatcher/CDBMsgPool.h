#pragma once

#include "CDBMsg.h"
#include "CArrayDBMsg.h"
#include "CSemaphoreChannel.h"

/**
 * CDBMsgPool�� CArrayDBMsg�� �̸� �Ҵ��� ��ü �޸𸮸� CSemaphoreChannel�� ������ �����ϰ� �����ϴ� ��Ƽ������ ��ü Ǯ�̴�.
 */
class CDBMsgPool
{
public:
	CDBMsgPool();
	~CDBMsgPool();	
public:
	DWORD	Open(DWORD pCount);
	VOID	Close();
	BOOL	IsOpen() const;
	CDBMsg*	Get(DWORD pTimeout = INFINITE);
	DWORD	Release(CDBMsg* pMsg);
	DWORD	GetSize() const;
	DWORD	GetFreeCount() const;
	BOOL	IsMember(const CDBMsg* pMsg) const;
private:
	CDBMsgPool(const CDBMsgPool&) = delete;
	CDBMsgPool& operator=(const CDBMsgPool&) = delete;
	VOID	__ResetAttr();
	VOID	__Dtor();
private:
	BOOL				__mIsOpen;
	CArrayDBMsg			__mAllMsg;					// Ǯ���� ������ CDBMsg ��ü ��ü�� �̸� �Ҵ��� �����ϴ� ���� ũ�� �迭
	CSemaphoreChannel	__mFreeSemaphoreChannel;	// ���� ������ CDBMsg ��ü�� ������ �����ϰ� �����ϴ� �������� ť
};