#include "SRInt.h"
#include "DB.h"

SRInt::SRInt(DB& db) 
	: db_(db) {
}

SRInt::~SRInt() {
}

void SRInt::operator()() {
	UserCommand command;
	while (true) {
		commands_queue_.pop(command);
		try {
			command();
		} catch (std::exception& e) {			
		}		
	}
}

void SRInt::dhCreate(const std::string& name) {
	commands_queue_.push(std::bind(&DB::create, db_, name));
}

void SRInt::dhFree(const std::string& name) {
	commands_queue_.push(std::bind(&DB::free, db_, name));
}

int64 SRInt::dhGet(const std::string& name) {
	return db_.get(name);
}

void SRInt::dhSet(const std::string& name, int64 value) {
	commands_queue_.push(std::bind(&DB::set, db_, name, value));
}

void SRInt::dhSetCallback(const std::string& name, NetworkCallback& callback) {
	commands_queue_.push(std::bind(&DB::setCallback, db_, name, callback));
}
