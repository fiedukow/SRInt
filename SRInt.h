#pragma once
#include <queue>
#include <functional>
#include "DBObserver.h"
#include "BlockingQueue.hpp"
#include "protobuf.pb.h"
#include "DB.h"
#include "zmq.hpp"
#include "Config.h"

typedef std::function<void ()> UserCommand;

class SRInt {
public:
	SRInt(DB& db);
	~SRInt();

	void operator()();

	// REQUESTED API
	void dhCreate(const std::string& name);
	void dhFree(const std::string& name);
	int64 dhGet(const std::string& name);
	void dhSet(const std::string& name, int64 value);
	void dhSetCallback(const std::string& name, NetworkCallback& callback);
	VariablesSnapshot dhGetSnapshot();

	void addObserver(DBObserver* observer);
	void removeObserver(DBObserver* observer);

private:
	void SendState();
	void SendEntryRequest();
	bool ReceiveMessage();
	void EngageSingleUserCommand();
	void UpdateConnection();

	DB& db_;
	std::queue<UserCommand> commands_queue_;

	zmq::context_t context_;
	zmq::socket_t client_;
	zmq::socket_t server_;	

	std::string last_connected_ip;
	int last_connected_port;

	Config cfg;
};

