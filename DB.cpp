#include "DB.h"
#include "protobuf.pb.h"

DB::DB(Message_State* baseState, const Message_NodeDescription* owner)
	: state_(baseState, owner), owner_node_(owner) {
}

DB::~DB() {
}

void DB::setState(Message_State* state) {
	state_ = StateHelper(state, owner_node_);
}

Message_State* DB::state() {
	return state_.state();
}

// Requested interface
void DB::create(const std::string& name) {
	if (state_.exists(name)) {
		state_.ensure_ownership(name);
		return;
	}
	
	state_.add_variable(name);
}

void DB::free(const std::string& name) {
	if (!state_.exists(name))
		return;

	state_.remove_ownership(name);
}

int64 DB::get(const std::string& name) {
	if (!state_.exists(name))
		throw std::runtime_error("DB::get: Variable does not exist.");	
	state_.ensure_ownership(name);
	return state_.get(name);
}

void DB::set(const std::string& name, int64 value) {
	if (!state_.exists(name))
		throw std::runtime_error("DB::set: Variable does not exist.");
	state_.ensure_ownership(name);
	state_.set(name, value);
}

void DB::setCallback(const std::string& name, NetworkCallback& callback) {
	DBObserverPtr observer(new NetworkAutoObserver(callback, name));
	addObserver(observer);
}

void DB::addObserver(DBObserverPtr observer) {
	observers_.push_back(observer);
}

void DB::removeObserver(DBObserverPtr observer) {
	observers_.remove(observer);
}

void DB::notifyLocalChangeObservers(const std::string& name) {
	for (DBObserverPtr observer : observers_)
		observer->LocalChangeNotify(name, *this);
}

void DB::notifyGlobalChangeObservers(const std::string& name) {
	for (DBObserverPtr observer : observers_)
		observer->GlobalChangeNotify(name, *this);
}

StateHelper::StateHelper(Message_State* state, const Message_NodeDescription* owner)
	: state_(state), owner_node_(owner) {
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

	var->mutable_owners()->Add()->CopyFrom(*owner_node_);
}

void StateHelper::remove_ownership(const std::string& name) {
	Message_Variable* var = find_variable(name);
	if (!var)
		return;
	
	for (int i = 0; i < var->owners_size(); ++i) {
		Message_NodeDescription* node = var->mutable_owners()->Mutable(i);
		if (node->ip() == owner_node_->ip() &&
			node->port() == owner_node_->port()) {
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

Message_Variable* StateHelper::find_variable(const std::string& name) {
	for (Message_Variable& var : *(state_->mutable_variables()))
		if (var.name() == name)
			return &var;
	return NULL;
}

