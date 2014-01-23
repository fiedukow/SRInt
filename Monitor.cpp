#include "Monitor.h"
#include <sstream>

const int kRetriesLimit = 5;

int Monitor::next_monitor_index = 0;

Monitor::Monitor(zmq::socket_t& socket, std::queue<int>& events)
: socket_(socket), events_(events), retries_(0) {
}

Monitor::~Monitor() {
}

void Monitor::operator()() {
	std::stringstream ss;
	ss << "inproc://monitor_" << next_monitor_index++ << ".req";
	std::string string = ss.str();
	monitor(socket_, string.c_str(), ZMQ_EVENT_ALL);
}

void Monitor::on_event_disconnected(const zmq_event_t &event_, const char* addr_) {
	events_.push(ZMQ_EVENT_DISCONNECTED);
}

void Monitor::on_event_connect_retried(const zmq_event_t &event_, const char* addr_) {
	if (retries_++ < kRetriesLimit) {
		return;
	}
	events_.push(ZMQ_EVENT_CONNECT_RETRIED);
}

void Monitor::on_event_connected(const zmq_event_t &event_, const char* addr_) {
	std::queue<int> empty;
	std::swap(events_, empty);
	retries_ = 0;
}