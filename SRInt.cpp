#include "SRInt.h"

SRInt::SRInt(DB& db) 
	: db_(db) {
}

SRInt::~SRInt() {
}

void SRInt::operator()() {
	while (true) {
		commands_queue_.front()(db_);
		commands_queue_.pop();
	}
}

void dhCreate(const std::string& name);
void dhFree(const std::string& name);
int64 dhGet(const std::string& name);
void dhSet(const std::string& name, int value);
void dhSetCallback(const std::string& name, NetworkCallback& callback);
