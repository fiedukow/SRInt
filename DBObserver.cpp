#include "DBObserver.h"
#include "DB.h"

NetworkAutoObserver::NetworkAutoObserver(NetworkCallback& callback, const std::string& nameFilter)
	: callback_(callback), nameFilter_(nameFilter) {
}

void NetworkAutoObserver::CallDoneNotify(const std::string& call_message, bool status, DB& db) {
	// Nothign to be done here.
}

void NetworkAutoObserver::LocalChangeNotify(const std::string& name, DB& db) {
	// Nothign to be done here.
}

void NetworkAutoObserver::GlobalChangeNotify(const std::string& name, int64 old_value, int64 new_value, DB& db) {
	if (nameFilter_ != name)
		return;
	callback_(name, db);
}