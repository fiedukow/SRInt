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

class Monitor : public zmq::monitor_t {
public:
	Monitor(zmq::socket_t& socket, std::queue<int>& events);
	virtual ~Monitor();
	void operator()();

	virtual void on_event_disconnected(const zmq_event_t &event_, const char* addr_);
	virtual void on_event_connect_delayed(const zmq_event_t &event_, const char* addr_);
	virtual void on_event_connected(const zmq_event_t &event_, const char* addr_);

private:
	zmq::socket_t& socket_;
	std::queue<int>& events_;
};

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
	enum ReceiveStatus { TOKEN_RECEIVED, NO_TOKEN_RECEIVED, RECEIVING_ERROR };

	void SendState();
	void SendEntryRequest();
	ReceiveStatus ReceiveMessage();	
	void EngageSingleUserCommand();
	void UpdateConnection();
	void HandleMonitorEvents();
	bool NetworkTokenShouldBeInitialized();

	DB& db_;
	std::queue<UserCommand> commands_queue_; // FIXME thread save (non-blocking) queue 

	zmq::context_t context_;
	zmq::socket_t client_;
	zmq::socket_t server_;	

	std::string last_connected_ip;
	int last_connected_port;

	// TODO Move this to another class
	std::queue<int> monitor_events_; // FIXME thread save (non-blocking) queue 
	Monitor monitor_;	

	Config cfg;
};

