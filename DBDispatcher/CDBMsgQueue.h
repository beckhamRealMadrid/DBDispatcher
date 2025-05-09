#pragma once

#include <windows.h>
#include <cassert>
#include <iostream>
#include <string>
#include "CDBMsg.h"

class CDBMsgQueue
{
public:
	CDBMsgQueue();
	~CDBMsgQueue();
public:
	DWORD	Open(DWORD pCapacity);
	VOID	Close();
	bool	IsOpen() const;
	DWORD	size() const;
	DWORD	max_size() const;
	bool	empty() const;
	bool	Full() const; // C++ 표준 컨테이너에는 함수가 존재하지 않기 때문에 첫문자가 대문자이다.
	CDBMsg&	front();
	CDBMsg&	back();
	bool	push_back(const CDBMsg& pMsg);
	bool	push_back(CDBMsg&& pMsg);
	bool	pop_front();
	bool	pop_back();
	void	clear();
private:
	CDBMsgQueue(const CDBMsgQueue&) = delete;
	CDBMsgQueue& operator=(const CDBMsgQueue&) = delete;
	VOID	__ResetAttr();
	VOID	__Dtor();
private:
	DWORD	__mMaxSize;		// 할당된 배열의 크기 (최대 저장 가능 개수)
	DWORD	__mSz;			// 현재 저장된 요소 개수
	DWORD	__mFrontIdx;	// 첫 번째 요소(가장 먼저 삽입된 값)가 위치하는 인덱스
	DWORD	__mRearIdx;		// 새로운 요소가 삽입될 인덱스
	CDBMsg*	__mArray;		// 동적으로 할당된 배열
};