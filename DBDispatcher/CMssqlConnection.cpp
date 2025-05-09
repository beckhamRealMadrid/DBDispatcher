#include "pch.h"
#include "CMssqlConnection.h"

CMssqlConnection::CMssqlConnection(const std::string& pServer,
	const std::string& pDatabase,
	const std::string& pUser,
	const std::string& pPassword)
	: __mServer(pServer), __mDatabase(pDatabase), __mUser(pUser), __mPassword(pPassword)
{
	// 연결은 Connect() 호출로 따로 실행
}

CMssqlConnection::~CMssqlConnection()
{
	Disconnect();
}

VOID CMssqlConnection::Connect()
{
	if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &__mEnv) != SQL_SUCCESS)
		throw std::runtime_error("Failed to allocate ODBC environment handle");

	if (SQLSetEnvAttr(__mEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0) != SQL_SUCCESS)
		throw std::runtime_error("Failed to set ODBC version");

	if (SQLAllocHandle(SQL_HANDLE_DBC, __mEnv, &__mDbc) != SQL_SUCCESS)
		throw std::runtime_error("Failed to allocate ODBC connection handle");

	std::string aConnStr = "DRIVER={ODBC Driver 17 for SQL Server};SERVER=" + __mServer +
		";DATABASE=" + __mDatabase +
		";UID=" + __mUser +
		";PWD=" + __mPassword + ";";

	SQLCHAR aOutConnStr[1024];
	SQLSMALLINT aOutConnStrLen;
	SQLRETURN aRet = SQLDriverConnectA(__mDbc, NULL,(SQLCHAR*)aConnStr.c_str(), SQL_NTS, aOutConnStr, sizeof(aOutConnStr),&aOutConnStrLen, SQL_DRIVER_NOPROMPT);

	if (aRet != SQL_SUCCESS && aRet != SQL_SUCCESS_WITH_INFO)
		__ThrowOnError("SQLDriverConnect", SQL_HANDLE_DBC, __mDbc);
}

VOID CMssqlConnection::Disconnect()
{
	if (__mStmt)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, __mStmt);
		__mStmt = nullptr;
	}
	if (__mDbc)
	{
		SQLDisconnect(__mDbc);
		SQLFreeHandle(SQL_HANDLE_DBC, __mDbc);
		__mDbc = nullptr;
	}
	if (__mEnv)
	{
		SQLFreeHandle(SQL_HANDLE_ENV, __mEnv);
		__mEnv = nullptr;
	}
}

VOID CMssqlConnection::Execute(const std::string& pQuery)
{
	if (!__mStmt)
	{
		if (SQLAllocHandle(SQL_HANDLE_STMT, __mDbc, &__mStmt) != SQL_SUCCESS)
			throw std::runtime_error("Failed to allocate statement handle");
	}

	SQLRETURN aRet = SQLExecDirectA(__mStmt, (SQLCHAR*)pQuery.c_str(), SQL_NTS);
	if (aRet != SQL_SUCCESS && aRet != SQL_SUCCESS_WITH_INFO)
		__ThrowOnError("SQLExecDirect", SQL_HANDLE_STMT, __mStmt);

	SQLCloseCursor(__mStmt);
}

std::vector<std::map<std::string, std::string>> CMssqlConnection::Query(const std::string& pQuery)
{
	if (!__mStmt)
	{
		if (SQLAllocHandle(SQL_HANDLE_STMT, __mDbc, &__mStmt) != SQL_SUCCESS)
			throw std::runtime_error("Failed to allocate statement handle");
	}

	SQLRETURN aRet = SQLExecDirectA(__mStmt, (SQLCHAR*)pQuery.c_str(), SQL_NTS);
	if (aRet != SQL_SUCCESS && aRet != SQL_SUCCESS_WITH_INFO)
		__ThrowOnError("SQLExecDirect", SQL_HANDLE_STMT, __mStmt);

	SQLSMALLINT aNumCols = 0;
	SQLNumResultCols(__mStmt, &aNumCols);

	std::vector<std::map<std::string, std::string>> aResults;

	while (SQLFetch(__mStmt) == SQL_SUCCESS)
	{
		std::map<std::string, std::string> aRow;
		for (SQLUSMALLINT aCol = 1; aCol <= aNumCols; ++aCol)
		{
			SQLCHAR aColName[128];
			SQLSMALLINT aColNameLen;
			SQLColAttribute(__mStmt, aCol, SQL_DESC_NAME, aColName, sizeof(aColName), &aColNameLen, NULL);

			SQLCHAR aBuf[1024];
			SQLLEN aIndicator;
			aRet = SQLGetData(__mStmt, aCol, SQL_C_CHAR, aBuf, sizeof(aBuf), &aIndicator);

			if (aRet == SQL_SUCCESS || aRet == SQL_SUCCESS_WITH_INFO)
			{
				if (aIndicator == SQL_NULL_DATA)
					aRow[(char*)aColName] = "NULL";
				else
					aRow[(char*)aColName] = (char*)aBuf;
			}
		}
		aResults.push_back(aRow);
	}

	SQLCloseCursor(__mStmt);
	return aResults;
}

VOID CMssqlConnection::__ThrowOnError(const std::string& pContext, SQLSMALLINT pHandleType, SQLHANDLE pHandle)
{
	SQLCHAR aSqlState[6], aMessage[256];
	SQLINTEGER aNativeError;
	SQLSMALLINT aTextLen;

	if (SQLGetDiagRecA(pHandleType, pHandle, 1, aSqlState, &aNativeError, aMessage, sizeof(aMessage), &aTextLen) == SQL_SUCCESS)
	{
		std::string aErrorMsg = pContext + " failed: [" + (char*)aSqlState + "] " + (char*)aMessage;
		throw std::runtime_error(aErrorMsg);
	}
	else
	{
		throw std::runtime_error(pContext + " failed: Unknown ODBC error");
	}
}