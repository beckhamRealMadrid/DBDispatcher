#pragma once

#include <vector>

class CDBMsg;

/**
 * CArrayDBMsg는 풀에서 재사용할 CDBMsg 객체들을 미리 할당해 관리하는 고정형 배열이다.
 */
class CArrayDBMsg
{
public:
	CArrayDBMsg();
	~CArrayDBMsg();	
public:
	DWORD			Open(DWORD pCount);
	VOID			Close();
	BOOL			IsOpen() const;
	const DWORD&	GetSize() const;
	CDBMsg*			Get(DWORD pIndex);
	const CDBMsg*	Get(DWORD pIndex) const;
	CDBMsg*&		operator[](int pIndex);
	const CDBMsg*	operator[](int pIndex) const;
	VOID			ForEach(const std::function<VOID(CDBMsg*)>& pFunc);
	VOID			ForEach(const std::function<VOID(const CDBMsg*)>& pFunc) const;
private:
	CArrayDBMsg(const CArrayDBMsg&) = delete;
	CArrayDBMsg& operator=(const CArrayDBMsg&) = delete;
	VOID			__ResetAttr();
private:
	std::vector<CDBMsg*>	__mArray;
	DWORD			__mSize;
};