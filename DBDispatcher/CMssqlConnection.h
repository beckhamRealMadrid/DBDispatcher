#pragma once

class CMssqlConnection
{
public:
	CMssqlConnection(const std::string& pServer,
		const std::string& pDatabase,
		const std::string& pUser,
		const std::string& pPassword);

	~CMssqlConnection();

	VOID Connect();
	VOID Disconnect();
	VOID Execute(const std::string& pQuery);
	std::vector<std::map<std::string, std::string>> Query(const std::string& pQuery);

private:
	VOID __ThrowOnError(const std::string& pContext, SQLSMALLINT pHandleType, SQLHANDLE pHandle);

private:
	SQLHENV __mEnv = nullptr;
	SQLHDBC __mDbc = nullptr;
	SQLHSTMT __mStmt = nullptr;

	std::string __mServer;
	std::string __mDatabase;
	std::string __mUser;
	std::string __mPassword;
};
