#pragma once
#include <queue>
#include "zmq.hpp"

class Monitor : public zmq::monitor_t {
public:
	Monitor(zmq::socket_t& socket, std::queue<int>& events);
	virtual ~Monitor();
	void operator()();

	virtual void on_event_disconnected(const zmq_event_t &event_, const char* addr_);
	virtual void on_event_connect_retried(const zmq_event_t &event_, const char* addr_);
	virtual void on_event_connected(const zmq_event_t &event_, const char* addr_);

private:
	zmq::socket_t& socket_;
	std::queue<int>& events_;

	int retries_;
	std::string last_connected_addr_; // zmq ftw!!!!!

	static int next_monitor_index;
};