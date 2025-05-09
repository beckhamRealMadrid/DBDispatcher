#pragma once

#include "CDBMsg.h"
#include "CArrayDBMsg.h"
#include "CSemaphoreChannel.h"

/**
 * CDBMsgPool은 CArrayDBMsg로 미리 할당한 객체 메모리를 CSemaphoreChannel로 스레드 안전하게 관리하는 멀티스레드 객체 풀이다.
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
	CArrayDBMsg			__mAllMsg;					// 풀에서 재사용할 CDBMsg 객체 전체를 미리 할당해 보관하는 고정 크기 배열
	CSemaphoreChannel	__mFreeSemaphoreChannel;	// 재사용 가능한 CDBMsg 객체를 스레드 안전하게 관리하는 세마포어 큐
};