#include "DBObserver.h"
#include "DB.h"

NetworkAutoObserver::NetworkAutoObserver(NetworkCallback& callback, const std::string& nameFilter)
	: callback_(callback), nameFilter_(nameFilter) {
}

void NetworkAutoObserver::LocalChangeNotify(const std::string& name, DB& db) {
	// Nothign to be done here.
}

void NetworkAutoObserver::GlobalChangeNotify(const std::string& name, DB& db) {
	if (nameFilter_ != name)
		return;
	callback_(name, db);
}