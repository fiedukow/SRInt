#pragma once
#include <string>
#include <functional>
#include <list>
#include <memory>

class DB;

class DBObserver {
public:
	virtual void CallDoneNotify(const std::string& call_message, bool status, DB& db) = 0;
	virtual void LocalChangeNotify(const std::string& name, DB& db) = 0;
	virtual void GlobalChangeNotify(const std::string& name, int old_value, int new_value, DB& db) = 0;
};

typedef std::shared_ptr<DBObserver> DBObserverPtr;

typedef std::function<void (const std::string&, DB&)> NetworkCallback;
class NetworkAutoObserver : public DBObserver {
public:
	NetworkAutoObserver(NetworkCallback& callback, const std::string& nameFilter);
	virtual void CallDoneNotify(const std::string& call_message, bool status, DB& db);
	virtual void LocalChangeNotify(const std::string& name, DB& db);
	virtual void GlobalChangeNotify(const std::string& name, int old_value, int new_value, DB& db);

private:
	const NetworkCallback callback_;
	const std::string nameFilter_;
};

