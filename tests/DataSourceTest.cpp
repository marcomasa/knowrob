/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <filesystem>
#include <gtest/gtest.h>
#include "knowrob/integration/python/utils.h"
#include "knowrob/ontologies/DataSource.h"
#include "knowrob/ontologies/OntologyFile.h"

using namespace knowrob;

// fixture class for testing
namespace knowrob::testing {
	class DataSourceTest : public ::testing::Test {
	protected:
		void SetUp() override {}
		// void TearDown() override {}
	};
}
using namespace knowrob::testing;

TEST_F(DataSourceTest, IsGraphVersionString) {
	EXPECT_TRUE(DataSource::isVersionString("v1.1"));
	EXPECT_TRUE(DataSource::isVersionString("v10.1.54"));
	EXPECT_TRUE(DataSource::isVersionString("1.1"));
	EXPECT_TRUE(DataSource::isVersionString("10.1.54"));
	EXPECT_FALSE(DataSource::isVersionString("10"));
	EXPECT_FALSE(DataSource::isVersionString("x10.54.3"));
	EXPECT_FALSE(DataSource::isVersionString("x.y.z"));
}

TEST_F(DataSourceTest, GraphNameFromURI) {
	EXPECT_EQ(DataSource::getNameFromURI("https://www.ontologydesignpatterns.org/ont/dul/DUL.owl"), "DUL");
	EXPECT_EQ(DataSource::getNameFromURI("file:///owl/SOMA.owl"), "SOMA");
	EXPECT_EQ(DataSource::getNameFromURI("./ont/SOMA.owl"), "SOMA");
	EXPECT_EQ(DataSource::getNameFromURI("SOMA.owl"), "SOMA");
	EXPECT_EQ(DataSource::getNameFromURI("SOMA"), "SOMA");
}

TEST_F(DataSourceTest, GraphVersionFromURI) {
	EXPECT_EQ(DataSource::getVersionFromURI("https://foo/v1.2.2/owl"), "v1.2.2");
}
