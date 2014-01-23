#include "Client.h"

#include <cassert>
#include "Monitor.h"

Client::Client(SafeQueue<int>& monitor_events, zmq::context_t& context)
	: monitor_events_(monitor_events), context_(context), socket_(NULL) {
}

Client::~Client() {
	if (connected_)
		disconnect();
}

void Client::connect(const std::string& address) {
	if (connected_ && address_ == address)
		return;
	disconnect();
	address_ = address;
	socket_ = new zmq::socket_t(context_, ZMQ_PUSH);
	monitor_ = new Monitor(*socket_, monitor_events_);
	monitor_thread_ = new std::thread(std::ref(*monitor_));
	socket_->connect(address_.c_str());
	connected_ = true;
}

void Client::disconnect() {
	if (!connected_)
		return;

	connected_ = false;
	monitor_->abort();
	monitor_thread_->join();
	delete monitor_thread_;
	delete monitor_;
	delete socket_;
	address_ = "";
	socket_ = NULL;
}

zmq::socket_t& Client::socket() {
	assert(hasSocket());
	return *socket_;
}

bool Client::hasSocket() {
	return (socket_ != NULL);
}