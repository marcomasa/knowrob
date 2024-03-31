#include "knowrob/storage/Storage.h"
#include "knowrob/py/utils.h"

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
		class_<Storage, std::shared_ptr<StorageWrap>, bases<DataSourceHandler>, boost::noncopyable>
				("Storage", init<>())
				// methods that must be implemented by backend plugins
				.def("initializeBackend", pure_virtual(&StorageWrap::initializeBackend))
				.def("insertOne", pure_virtual(&StorageWrap::insertOne))
				.def("insertAll", pure_virtual(&StorageWrap::insertAll))
				.def("removeAll", pure_virtual(&StorageWrap::removeAll))
				.def("removeOne", pure_virtual(&StorageWrap::removeOne))
				.def("removeAllWithOrigin", pure_virtual(&StorageWrap::removeAllWithOrigin));
	}
}
