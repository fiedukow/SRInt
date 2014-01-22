#pragma once
#include <queue>
#include <thread>

#include "zmq.hpp"

class Monitor;

class Client {
public:
	Client(std::queue<int>& monitor_events, zmq::context_t& context);
	~Client();

	void connect(const std::string& address);
	void disconnect();

	zmq::socket_t& socket();
	bool hasSocket();

private:
	std::string address_;
	std::queue<int>& monitor_events_;
	zmq::context_t& context_;
	bool connected_;

	std::thread* monitor_thread_;
	zmq::socket_t* socket_;
	Monitor* monitor_;
};

