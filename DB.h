#pragma once
#include "DBObserver.h"
#include <list>
#include <memory>

class Message_State;
class Message_NodeDescription;
class Message_Variable;
class DBObserver;
typedef __int64 int64;

class StateHelper {
public:
	StateHelper(Message_State* state, const Message_NodeDescription* owner);
	bool exists(const std::string& name);
	void ensure_ownership(const std::string& name);
	void remove_ownership(const std::string& name);
	void add_variable(const std::string& name, int64 value = 0);
	void set(const std::string& name, int64 value);
	int64 get(const std::string& name);
	bool is_owner(const std::string& name);
	Message_State* state();

private:
	Message_Variable* find_variable(const std::string& name);

	Message_State* state_;
	const Message_NodeDescription* owner_node_;
};

class DB {
public:
	DB(Message_State* baseState, const Message_NodeDescription* owner);
	~DB();

	void setState(Message_State*);
	Message_State* state();

	void create(const std::string& name);
	void free(const std::string& name);
	int64 get(const std::string& name);
	void set(const std::string& name, int64 value);
	void setCallback(const std::string& name, NetworkCallback& callback);

	void addObserver(DBObserverPtr observer);
	void removeObserver(DBObserverPtr observer);

private:
	void notifyLocalChangeObservers(const std::string& name);
	void notifyGlobalChangeObservers(const std::string& name);

	std::list<DBObserverPtr> observers_;
	StateHelper state_;
	const Message_NodeDescription* owner_node_;
};