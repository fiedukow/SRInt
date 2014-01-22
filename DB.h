#pragma once
#include "DBObserver.h"
#include <list>
#include <memory>
#include <vector>

class Message_State;
class Message_NodeDescription;
class Message_Variable;
class DBObserver;
typedef __int64 int64;

typedef std::vector<std::pair<std::string, int64>> VariablesSnapshot;

class StateHelper {
public:
	struct Change {
		std::string name;
		int64 before;
		int64 after;
	};
	typedef std::vector<Change> Changes;

	StateHelper(Message_State* state, const Message_NodeDescription* owner);
	~StateHelper();

	bool exists(const std::string& name);
	void ensure_ownership(const std::string& name);
	void remove_ownership(const std::string& name);
	void add_variable(const std::string& name, int64 value = 0);
	void set(const std::string& name, int64 value);
	int64 get(const std::string& name);
	void add_node(Message_NodeDescription* new_node);
	void remove_follower();

	const Message_NodeDescription* next_node();

	VariablesSnapshot state_snapshot();
	bool is_owner(const std::string& name);
	Message_State* state();

	Changes StateHelper::diff(StateHelper& new_helper);

private:
	Message_Variable* find_variable(const std::string& name);

public:
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
	VariablesSnapshot state_snapshot();
	void addNode(Message_NodeDescription* node);
	void removeFollower();
	int increaseStateId(); //returns new id

	const Message_NodeDescription* nextNode();

	void addObserver(DBObserverPtr observer);
	void removeObserver(DBObserverPtr observer);
	void addObserver(DBObserver* observer);
	void removeObserver(DBObserver* observer);

private:
	void notifyCallDoneObservers(const std::string& command, bool status);
	void notifyLocalChangeObservers(const std::string& name);
	void notifyGlobalChangeObservers(const std::string& name, int64 old_value, int64 new_value);

	void diffStatesAndNotifyObserver(StateHelper& new_state);

	std::list<DBObserverPtr> owned_observers_;
	std::list<DBObserver*> observers_;
	StateHelper state_;
	const Message_NodeDescription* owner_node_;
};