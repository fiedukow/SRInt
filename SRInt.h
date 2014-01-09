#pragma once
#include <queue>
#include <functional>
#include "DBObserver.h"

class DB;
typedef std::function<void (DB&)> UserCommand;
typedef __int64 int64;

class SRInt {
public:
	SRInt(DB& db);
	~SRInt();

	void operator()();

	// REQUESTED API
	void dhCreate(const std::string& name);
	void dhFree(const std::string& name);
	int64 dhGet(const std::string& name);
	void dhSet(const std::string& name, int value);
	void dhSetCallback(const std::string& name, NetworkCallback& callback);

private:
	DB& db_;
	std::queue<UserCommand> commands_queue_;
};

