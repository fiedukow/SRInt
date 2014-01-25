#include "DB.h"
#include "protobuf.pb.h"
#include <sstream>
#include <iostream>
#include <algorithm>

DB::DB(Message_State* baseState, const Message_NodeDescription* owner)
	: state_(baseState, owner), owner_node_(owner) {
}

DB::~DB() {
}

void DB::setState(Message_State* state) {
	StateHelper new_state(state, owner_node_);
	diffStatesAndNotifyObserver(new_state);
	Message_State* old_state = state_.state();
	state_ = new_state;
	delete old_state;
}

Message_State* DB::state() {
	return state_.state();
}

// Requested interface
void DB::create(const std::string& name) {
	std::string done_message = std::string("Created ") + name;	
	if (state_.exists(name)) {
		state_.ensure_ownership(name);
		notifyCallDoneObservers(done_message, false);
		return;
	}
	
	state_.add_variable(name);
	notifyCallDoneObservers(done_message, true);
}

void DB::free(const std::string& name) {
	std::string done_message = std::string("Free ") + name;	
	if (!state_.exists(name)) {
		notifyCallDoneObservers(done_message, false);
		return;
	}

	notifyCallDoneObservers(done_message, true);
	state_.remove_ownership(name);
}

int64 DB::get(const std::string& name) {
	std::string done_message = std::string("Variable ") + name + std::string(" obtained");
	if (!state_.exists(name)) {
		notifyCallDoneObservers(done_message, false);
		throw std::runtime_error("DB::get error");
	}
	notifyCallDoneObservers(done_message, true);
	return state_.get(name);
}

void DB::set(const std::string& name, int64 value) {
	std::stringstream ss;
	ss << "Set " << name << " to " << value;
	std::string done_message = ss.str();	

	if (!state_.exists(name)) {
		notifyCallDoneObservers(done_message, false);
		return;
	}

	state_.ensure_ownership(name);
	state_.set(name, value);
	notifyCallDoneObservers(done_message, true);
}

void DB::setCallback(const std::string& name, NetworkCallback& callback) {
	std::string done_message = std::string("Callback on ") + name + std::string(" created");	

	DBObserverPtr observer(new NetworkAutoObserver(callback, name));
	addObserver(observer);
	notifyCallDoneObservers(done_message, true);
}

VariablesSnapshot DB::state_snapshot() {
	std::string done_message = "Snapshot obtained";
	notifyCallDoneObservers(done_message, true);
	return state_.state_snapshot();
}

void DB::addNode(Message_NodeDescription* node) {
	state_.add_node(node);
}

void DB::removeFollower() {
	state_.remove_follower();
}

int64 DB::increaseStateId(int increase_by) {
	state()->set_state_id(state()->state_id() + increase_by);
	return state()->state_id();
}

const Message_NodeDescription* DB::nextNode() {
	return state_.next_node();
}

void DB::addObserver(DBObserver* observer) {
	observers_.push_back(observer);
}

void DB::removeObserver(DBObserver* observer) {
	observers_.remove(observer);
}

void DB::addObserver(DBObserverPtr observer) {
	owned_observers_.push_back(observer);
}

void DB::removeObserver(DBObserverPtr observer) {
	owned_observers_.remove(observer);
}

void DB::diffStatesAndNotifyObserver(StateHelper& new_state) {
	StateHelper::Changes changes = state_.diff(new_state);
	for(StateHelper::Change& change : changes) {
		notifyGlobalChangeObservers(change.name, change.before, change.after);
	}
}

void DB::notifyCallDoneObservers(const std::string& command, bool status) {
	for (DBObserver* observer : observers_)
		observer->CallDoneNotify(command, status, *this);
	for (DBObserverPtr observer : owned_observers_)
		observer->CallDoneNotify(command, status, *this);
}

void DB::notifyLocalChangeObservers(const std::string& name) {
	for (DBObserver* observer : observers_)
		observer->LocalChangeNotify(name, *this);
	for (DBObserverPtr observer : owned_observers_)
		observer->LocalChangeNotify(name, *this);
}

void DB::notifyGlobalChangeObservers(const std::string& name, int64 old_value, int64 new_value) {
	for (DBObserver* observer : observers_)
		observer->GlobalChangeNotify(name, old_value, new_value, *this);
	for (DBObserverPtr observer : owned_observers_)
		observer->GlobalChangeNotify(name, old_value, new_value, *this);
}

StateHelper::StateHelper(Message_State* state, const Message_NodeDescription* owner)
	: state_(state), owner_node_(owner) {
}

StateHelper::~StateHelper() {
}

bool StateHelper::exists(const std::string& name) {
	return find_variable(name) != NULL;
}

void StateHelper::ensure_ownership(const std::string& name) {
	Message_Variable* var = find_variable(name);
	if (!var)
		return;
	
	if (is_owner(name))
		return;

	int b = var->owners_size();
	var->mutable_owners()->Add()->CopyFrom(*owner_node_);
	int a = var->owners_size();
}

void StateHelper::remove_ownership(const std::string& name) {
	remove_ownership(name, owner_node_);
}

void StateHelper::add_variable(const std::string& name, int64 value) {
	Message_Variable* var = new Message_Variable;
	var->set_name(name);
	var->set_value(value);
	state_->mutable_variables()->AddAllocated(var);
	ensure_ownership(name);
}

void StateHelper::set(const std::string& name, int64 value) {
	Message_Variable* var = find_variable(name);
	assert(var);
	var->set_value(value);
}

int64 StateHelper::get(const std::string& name) {
	Message_Variable* var = find_variable(name);
	assert(var);
	return var->value();
}

void StateHelper::add_node(Message_NodeDescription* new_node) {
	std::list<Message_NodeDescription*> nodes;
	int max_node_id = -1;
	while (state_->nodes_size() > 0) {
		Message_NodeDescription* current = state_->mutable_nodes()->ReleaseLast();
		if (current->ip() == owner_node_->ip() &&
			current->port() == owner_node_->port()) {
			nodes.push_front(new_node);
		}
		nodes.push_front(current);
		max_node_id = std::max(max_node_id, current->node_id());
	}
	new_node->set_node_id(max_node_id + 1);

	assert(std::find(nodes.begin(), nodes.end(), new_node) != nodes.end());
	for (Message_NodeDescription* node : nodes)
		state_->mutable_nodes()->AddAllocated(node);
}

void StateHelper::remove_follower() {
	assert(state_->nodes_size() > 1);
	std::list<Message_NodeDescription*> new_nodes;
	Message_NodeDescription* removed = NULL;

	while (state_->nodes_size() > 0) {
		Message_NodeDescription* current = state_->mutable_nodes()->ReleaseLast();		
		if (current->ip() == owner_node_->ip() &&
			current->port() == owner_node_->port()) {
			assert(removed == NULL);
			if (new_nodes.size() > 0) {
				removed = new_nodes.front();
				new_nodes.pop_front();
			}
		}
		new_nodes.push_front(current);
	}
	if (removed == NULL) {
		removed = new_nodes.front();
		new_nodes.pop_front();
	}
	on_node_removed(removed);

	assert(std::find_if(new_nodes.begin(),
						new_nodes.end(),
						[this](const Message_NodeDescription* desc) {
							return desc->ip() == owner_node_->ip() &&
								   desc->port() == owner_node_->port();
						} ) != new_nodes.end());
	for (Message_NodeDescription* node : new_nodes)
		state_->mutable_nodes()->AddAllocated(node);
}

const Message_NodeDescription* StateHelper::next_node() {
	assert(state_->nodes_size() > 0);

	auto i = state_->nodes().begin();
	for (; (i+1) != state_->nodes().end(); ++i) {
		if (i->ip() == owner_node_->ip() &&
			i->port() == owner_node_->port()) {
			return &(*(i+1));
		}
	}
	assert(i->ip() == owner_node_->ip() && i->port() == owner_node_->port());
	return &(*state_->nodes().begin());
}

VariablesSnapshot StateHelper::state_snapshot() {
	VariablesSnapshot snapshot;
	for (Message_Variable& var : *(state_->mutable_variables()))
		snapshot.push_back(std::make_pair(var.name(), var.value()));

	return snapshot;
}

bool StateHelper::is_owner(const std::string& name) {
	Message_Variable* var = find_variable(name);
	if (!var)
		return false;
	
	for (const Message_NodeDescription& node : var->owners()) {
		if (node.ip() == owner_node_->ip() &&
			node.port() == owner_node_->port())			
			return true;
	}
	return false;
}

Message_State* StateHelper::state() {
	return state_;
}

StateHelper::Changes StateHelper::diff(StateHelper& new_helper) {
	std::vector<StateHelper::Change> changes;
	for (Message_Variable& var : *(state_->mutable_variables())) {
		Message_Variable* new_var = new_helper.find_variable(var.name());
		if (new_var && new_var->value() != var.value())
			changes.push_back(Change({ var.name(), var.value(), new_var->value() }));
	}
	return changes;
}

Message_Variable* StateHelper::find_variable(const std::string& name) {
	for (Message_Variable& var : *(state_->mutable_variables()))
		if (var.name() == name)
			return &var;
	return NULL;
}

void StateHelper::remove_ownership(const std::string& name, const Message_NodeDescription* node) {
	Message_Variable* var = find_variable(name);
	if (!var)
		return;

	for (int i = 0; i < var->owners_size(); ++i) {
		Message_NodeDescription* current_node = var->mutable_owners()->Mutable(i);
		if (current_node->ip() == node->ip() &&
			current_node->port() == node->port()) {
			var->mutable_owners()->SwapElements(i, var->owners_size() - 1);
			var->mutable_owners()->RemoveLast();
			--i;
		}
	}

	if (var->owners_size() > 0)
		return;

	for (int i = 0; i < state_->variables_size(); ++i) {
		Message_Variable* currentVar = state_->mutable_variables()->Mutable(i);
		if (currentVar == var) {
			state_->mutable_variables()->SwapElements(i, state_->variables_size() - 1);
			state_->mutable_variables()->RemoveLast();
			--i;
		}
	}
}

void StateHelper::on_node_removed(const Message_NodeDescription* node) {
	std::list<std::string> names_to_remove_ownership;
	for (Message_Variable& var : *(state_->mutable_variables()))
		names_to_remove_ownership.push_back(var.name());
	for (const std::string& name : names_to_remove_ownership)
		remove_ownership(name, node);
}

