#pragma once

#include "CSemaphoreChannel.h"
#include "CUserDBQueue.h"
#include "CDBMsgPool.h"

const DWORD ON_MAX_CONNECTION = 4300;
const DWORD USER_DB_QUEUE_CAPACITY = 100;
const DWORD DB_REQUEST_POOL_CAPACITY = 10000;

class CMssqlConnection;

class CDBDispatcher
{
private:
	class CWorkerInfo
	{
	public:
		CWorkerInfo(HANDLE pThreadHandle, DWORD pThreadId) : mThread(pThreadHandle), mThreadId(pThreadId) {}

		HANDLE GetHandle() const { return mThread; }
		DWORD GetThreadId() const { return mThreadId; }

		void Close()
		{
			if (mThread)
			{
				CloseHandle(mThread);
				mThread = nullptr;
			}
		}

	private:
		HANDLE mThread;
		DWORD mThreadId;
	};
public:
	CDBDispatcher();
	~CDBDispatcher();
	static	CDBDispatcher&	This();
	static unsigned int __stdcall	ThreadEntryPoint(VOID* pArg);
public:
	DWORD			Open();
	VOID			Close();
	DWORD			QueueUserRequest(WORD pAccount, WORD pCommand, const void* pPayload, DWORD pSize);
	DWORD			QueueSharedRequest(WORD pCommand, const void* pPayload, DWORD pSize);
	DWORD			RunDispatchLoop();	
private:
	VOID			__ResetAttr();
	VOID			__Dtor();
	VOID			__BindDB();
	CMssqlConnection*	__GetSql();
	DWORD			__PushRequestToWorker(CDBMsg* pMsg);
	VOID			__ExecuteRequest(WORD pCommand, const char* pData);
	VOID			__ProcessUserQueue(DWORD pAccount);
	CDBMsg*			__GetFromPool();
	VOID			__ReleaseToPool(CDBMsg* pMsg);
	CUserDBQueue*	__GetUserDBQueueByAccount(DWORD pAccount);
private:
	typedef std::vector<CWorkerInfo>					__TWorker;
	typedef std::unordered_map<DWORD, DWORD>			__TLinker;
	typedef CLockerAuto<CLocker>						__TLockerAuto;
	typedef std::unordered_map<DWORD, CUserDBQueue*>	__TUserDBQueueMap;
	typedef std::vector<CMssqlConnection*>				__TSqlPool;
private:
	static	CDBDispatcher	__mSingleton;
private:
	__TLockerAuto::TLocker	__mLocker;
	__TWorker				__mWorkers;
	__TUserDBQueueMap		__mUserDBQueueMap;
	__TSqlPool				__mSqlPool;	
	__TLinker				__mLinker;
	CSemaphoreChannel		__mSemaphoreChannel;
	CDBMsgPool				__mDBMsgPool;
	INT						__mWorkerCount;
	DWORD					__mLinkCnt;	
};