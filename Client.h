#pragma once
#include <queue>
#include <thread>

#include "zmq.hpp"
#include "SafeQueue.hpp"

class Monitor;

class Client {
public:
	Client(SafeQueue<int>& monitor_events, zmq::context_t& context);
	~Client();

	void connect(const std::string& address);
	void disconnect();

	zmq::socket_t& socket();
	bool hasSocket();

private:
	std::string address_;
	SafeQueue<int>& monitor_events_;
	zmq::context_t& context_;
	bool connected_;

	std::thread* monitor_thread_;
	zmq::socket_t* socket_;
	Monitor* monitor_;
};

