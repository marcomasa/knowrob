/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <filesystem>

#include "knowrob/Logger.h"
#include "knowrob/storage/StorageManager.h"
#include "knowrob/storage/StorageError.h"
#include "knowrob/storage/QueryableStorage.h"

using namespace knowrob;

StorageManager::StorageManager(const std::shared_ptr<Vocabulary> &vocabulary)
		: PluginManager(),
		  vocabulary_(vocabulary) {
}

std::shared_ptr<NamedBackend> StorageManager::loadPlugin(const boost::property_tree::ptree &config) {
	// get a backend factory
	std::shared_ptr<BackendFactory> factory = findFactory(config);
	// make sure factory was found above
	if (!factory) throw StorageError("failed to load a backend.");
	// create a reasoner id, or use name property
	std::string backendID = getPluginID(factory, config);
	KB_INFO("Using backend `{}` with type `{}`.", backendID, factory->name());

	// create a new DataBackend instance
	auto definedBackend = factory->create(backendID);
	definedBackend->value()->setVocabulary(vocabulary());

	PropertyTree pluginConfig(&config);
	if (!definedBackend->value()->initializeBackend(pluginConfig)) {
		KB_WARN("Backend `{}` failed to loadConfig.", backendID);
	} else {
		addPlugin(definedBackend);
	}

	return definedBackend;
}

std::shared_ptr<NamedBackend> StorageManager::addPlugin(std::string_view backendID, const StoragePtr &backend) {
	if (pluginPool_.find(backendID) != pluginPool_.end()) {
		KB_WARN("Overwriting backend with name '{}'", backendID);
	}
	auto managedBackend = std::make_shared<NamedBackend>(backendID, backend);
	pluginPool_.emplace(managedBackend->name(), managedBackend);
	initBackend(managedBackend);
	return managedBackend;
}

void StorageManager::addPlugin(const std::shared_ptr<NamedBackend> &definedKG) {
	if (pluginPool_.find(definedKG->name()) != pluginPool_.end()) {
		KB_WARN("Overwriting backend with name '{}'", definedKG->name());
	}
	pluginPool_[definedKG->name()] = definedKG;
	initBackend(definedKG);
}

void StorageManager::initBackend(const std::shared_ptr<NamedBackend> &definedKG) {
	definedKG->value()->setVocabulary(vocabulary());
	// check if the backend is a QueryableBackend, if so store it in the queryable_ map
	auto queryable = std::dynamic_pointer_cast<QueryableStorage>(definedKG->value());
	if (queryable) {
		KB_INFO("Using queryable backend with id '{}'.", definedKG->name());
		queryable_[definedKG->name()] = queryable;
	}
	// check if the backend is a PersistentBackend, if so store it in the persistent_ map
	if (queryable && queryable->isPersistent()) {
		KB_INFO("Using persistent backend with id '{}'.", definedKG->name());
		persistent_[definedKG->name()] = queryable;
	}
}
