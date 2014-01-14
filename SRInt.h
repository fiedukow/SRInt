#pragma once
#include <queue>
#include <functional>
#include "DBObserver.h"
#include "BlockingQueue.hpp"
#include "protobuf.pb.h"

class DB;
typedef std::function<void ()> UserCommand;
typedef __int64 int64;

class SRInt {
public:
	class Observer {
		void CommandApplied(const std::string& command);
		void StateReceived();
	};

	SRInt(DB& db);
	~SRInt();

	void operator()();

	// REQUESTED API
	void dhCreate(const std::string& name);
	void dhFree(const std::string& name);
	int64 dhGet(const std::string& name);
	void dhSet(const std::string& name, int64 value);
	void dhSetCallback(const std::string& name, NetworkCallback& callback);

private:
	void SendState();
	void ReceiveState();
	void EngageSingleUserCommand();

	DB& db_;
	std::queue<UserCommand> commands_queue_;
};

