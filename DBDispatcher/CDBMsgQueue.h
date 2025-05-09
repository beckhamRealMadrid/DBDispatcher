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
	bool	Full() const; // C++ ǥ�� �����̳ʿ��� �Լ��� �������� �ʱ� ������ ù���ڰ� �빮���̴�.
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
	DWORD	__mMaxSize;		// �Ҵ�� �迭�� ũ�� (�ִ� ���� ���� ����)
	DWORD	__mSz;			// ���� ����� ��� ����
	DWORD	__mFrontIdx;	// ù ��° ���(���� ���� ���Ե� ��)�� ��ġ�ϴ� �ε���
	DWORD	__mRearIdx;		// ���ο� ��Ұ� ���Ե� �ε���
	CDBMsg*	__mArray;		// �������� �Ҵ�� �迭
};