/*
 * Copyright (c) 2022, Daniel Beßler
 * All rights reserved.
 *
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <filesystem>
#include <utility>

#include "knowrob/Logger.h"
#include "knowrob/reasoner/ReasonerManager.h"
#include "knowrob/reasoner/ReasonerError.h"

using namespace knowrob;

std::map<std::string, std::shared_ptr<ReasonerFactory>> ReasonerManager::reasonerFactories_ = {};
std::map<uint32_t, ReasonerManager*> ReasonerManager::reasonerManagers_ = {};
uint32_t ReasonerManager::managerIDCounter_ = 0;

ReasonerManager::ReasonerManager(const std::shared_ptr<ThreadPool> &threadPool,
                                 const std::shared_ptr<KnowledgeGraphManager> &backendManager)
: threadPool_(threadPool),
  backendManager_(backendManager),
  reasonerIndex_(0)
{
    std::lock_guard<std::mutex> scoped_lock(staticMutex_);
    managerID_ = (managerIDCounter_++);
    reasonerManagers_[managerID_] = this;
}

ReasonerManager::~ReasonerManager()
{
    std::lock_guard<std::mutex> scoped_lock(staticMutex_);
    reasonerManagers_.erase(managerID_);
}

ReasonerManager* ReasonerManager::getReasonerManager(uint32_t managerID)
{
    auto it = reasonerManagers_.find(managerID);
    if(it != reasonerManagers_.end()) {
        return it->second;
    } else {
        return nullptr;
    }
}

void ReasonerManager::loadReasoner(const boost::property_tree::ptree &config)
{
	auto lib = config.get_optional<std::string>("lib");
	auto type = config.get_optional<std::string>("type");
	auto name = config.get_optional<std::string>("name");
	auto backendName = config.get_optional<std::string>("data-backend");

	// get a reasoner factory
	std::shared_ptr<ReasonerFactory> factory;
	if(lib.has_value()) {
		// use factory in DLL
		factory = loadReasonerPlugin(lib.value());
	}
	else if(type.has_value()) {
		// map type name to a factory
		const auto &it = reasonerFactories_.find(type.value());
		if(it == reasonerFactories_.end()) {
			KB_WARN("no factory registered for reasoner type '{}'.", type.value());
		}
		else {
			factory = it->second;
		}
	}
	else {
		KB_WARN("missing 'type' or 'lib' key in reasoner config.");
	}
	// make sure factory was found above
	if(!factory) {
		throw ReasonerError("failed to load a reasoner.");
	}
	// create a reasoner id, or use name property
	std::string reasonerID;
	if(name.has_value()) {
		reasonerID = name.value();
	}
	else {
		reasonerID = factory->name() + std::to_string(reasonerIndex_);
	}
	KB_INFO("Using reasoner `{}` with type `{}`.", reasonerID, factory->name());
	// create a new reasoner instance
	auto reasoner = factory->createReasoner(reasonerID);
    // reasoner need to have a reference to the reasoner manager such that
    // predicates can be defined that interact with other reasoner subsystems.
    reasoner->setReasonerManager(managerID_);

	if(backendName.has_value()) {
	    auto backend = backendManager_->getKnowledgeGraphWithID(backendName.value());
	    if(backend) {
            reasoner->setDataBackend(backend->knowledgeGraph());
	    }
	    else {
		    throw ReasonerError("Reasoner `{}` refers to unknown data-backend `{}`.", reasonerID, backendName.value());
	    }
	}
	else {
        throw ReasonerError("Reasoner `{}` has no 'data-backend' configured.", reasonerID);
	}
    addReasoner(reasonerID, reasoner);

	ReasonerConfiguration reasonerConfig;
	reasonerConfig.loadPropertyTree(&config);
	if(!reasoner->loadConfiguration(reasonerConfig)) {
		KB_WARN("Reasoner `{}` failed to loadConfiguration.", reasonerID);
	}
	else {
        for(auto &dataSource : reasonerConfig.dataSources) {
            if(!reasoner->loadDataSource(dataSource)) {
                KB_WARN("Reasoner `{}` failed to load data source {}.", reasonerID, dataSource->uri());
            }
        }
	}
	// increase reasonerIndex_
	reasonerIndex_ += 1;
}

std::shared_ptr<ReasonerPlugin> ReasonerManager::loadReasonerPlugin(const std::string &path)
{
	auto absPath = std::filesystem::absolute(path);
	auto it = loadedPlugins_.find(absPath);
	if(it == loadedPlugins_.end()) {
		auto p = std::make_shared<ReasonerPlugin>(absPath);
		auto jt = loadedPlugins_.insert(std::pair<std::string,
										std::shared_ptr<ReasonerPlugin>>(absPath, p));
		if(jt.first->second->loadDLL()) {
			return jt.first->second;
		}
	}
	else if(it->second->isLoaded()) {
		return it->second;
	}
	KB_WARN("Failed to open reasoner library at path '{}'.", path);
	return {};
}

bool ReasonerManager::addReasonerFactory(const std::string &typeName, const std::shared_ptr<ReasonerFactory> &factory)
{
	if(reasonerFactories_.find(typeName) != reasonerFactories_.end()) {
		KB_WARN("overwriting factory for reasoner type '{}'", typeName);
	}
	reasonerFactories_[typeName] = factory;
	return true;
}

std::shared_ptr<DefinedReasoner> ReasonerManager::addReasoner(
        const std::string &reasonerID, const std::shared_ptr<Reasoner> &reasoner)
{
	if(reasonerPool_.find(reasonerID) != reasonerPool_.end()) {
		KB_WARN("overwriting reasoner with name '{}'", reasonerID);
	}
	auto managedReasoner = std::make_shared<DefinedReasoner>(reasonerID, reasoner);
	reasonerPool_[reasonerID] = managedReasoner;
    reasoner->setReasonerManager(managerID_);

	return managedReasoner;
}

void ReasonerManager::removeReasoner(const std::shared_ptr<DefinedReasoner> &reasoner)
{
	reasonerPool_.erase(reasoner->name());
}

std::shared_ptr<DefinedReasoner> ReasonerManager::getReasonerWithID(const std::string &reasonerID)
{
	auto it = reasonerPool_.find(reasonerID);
	if(it != reasonerPool_.end()) {
		return it->second;
	}
	else {
		return {};
	}
}

std::shared_ptr<DefinedPredicate> ReasonerManager::getPredicateDefinition(
		const std::shared_ptr<PredicateIndicator> &indicator)
{
	auto description = std::make_shared<DefinedPredicate>(indicator);
	for(auto &x : reasonerPool_) {
		auto description_n = x.second->reasoner()->getPredicateDescription(indicator);
		if(description_n && !description->addReasoner(x.second, description_n)) {
			KB_WARN("ignoring inconsistent reasoner descriptions provided.");
		}
	}
	return description;
}
