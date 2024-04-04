#include "knowrob/storage/Storage.h"
#include "knowrob/integration/python/utils.h"
#include "knowrob/storage/QueryableStorage.h"

using namespace knowrob;

StorageFeature knowrob::operator|(StorageFeature a, StorageFeature b) {
	return static_cast<StorageFeature>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

bool knowrob::operator&(StorageFeature a, StorageFeature b) {
	return static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b);
}

namespace knowrob::py {
	struct StorageWrap : public Storage, boost::python::wrapper<Storage> {
		explicit StorageWrap(PyObject *p) : self(p), Storage() {}

		bool initializeBackend(const PropertyTree &config) override {
			return call_method<bool>(self, "initializeBackend", config);
		}

		bool insertOne(const FramedTriple &triple) override {
			return call_method<bool>(self, "insertOne", &triple);
		}

		bool insertAll(const TripleContainerPtr &triples) override {
			return call_method<bool>(self, "insertAll", triples);
		}

		bool removeOne(const FramedTriple &triple) override {
			return call_method<bool>(self, "removeOne", &triple);
		}

		bool removeAll(const TripleContainerPtr &triples) override {
			return call_method<bool>(self, "removeAll", triples);
		}

		bool removeAllWithOrigin(std::string_view origin) override {
			return call_method<bool>(self, "removeAllWithOrigin", origin.data());
		}

	private:
		PyObject *self;
	};

	template<>
	void createType<Storage>() {
		using namespace boost::python;
		enum_<StorageFeature>("StorageFeature")
				.value("NothingSpecial", StorageFeature::NothingSpecial)
				.value("ReAssignment", StorageFeature::ReAssignment)
				.value("TripleContext", StorageFeature::TripleContext)
				.export_values();
		class_<Storage, std::shared_ptr<StorageWrap>, bases<DataSourceHandler>, boost::noncopyable>
				("Storage", init<>())
				.def("vocabulary", &Storage::vocabulary, return_value_policy<reference_existing_object>())
				.def("supports", &Storage::supports)
				.def("getVersionOfOrigin", &Storage::getVersionOfOrigin)
				.def("setVersionOfOrigin", &Storage::setVersionOfOrigin)
						// methods that must be implemented by backend plugins
				.def("initializeBackend", pure_virtual(&Storage::initializeBackend))
				.def("insertOne", pure_virtual(&Storage::insertOne))
				.def("insertAll", pure_virtual(&Storage::insertAll))
				.def("removeAll", pure_virtual(&Storage::removeAll))
				.def("removeOne", pure_virtual(&Storage::removeOne))
				.def("removeAllWithOrigin", pure_virtual(&Storage::removeAllWithOrigin));
		createType<QueryableStorage>();
	}
}
