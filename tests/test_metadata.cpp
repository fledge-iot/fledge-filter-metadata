#include <gtest/gtest.h>
#include <plugin_api.h>
#include <config_category.h>
#include <filter_plugin.h>
#include <filter.h>
#include <string.h>
#include <string>
#include <rapidjson/document.h>
#include <reading.h>
#include <reading_set.h>
#include <logger.h>

using namespace std;
using namespace rapidjson;

extern "C"
{
	PLUGIN_INFORMATION *plugin_info();
	void plugin_ingest(void *handle,
					   READINGSET *readingSet);
	PLUGIN_HANDLE plugin_init(ConfigCategory *config,
							  OUTPUT_HANDLE *outHandle,
							  OUTPUT_STREAM output);
	int called = 0;

	void Handler(void *handle, READINGSET *readings)
	{
		called++;
		*(READINGSET **)handle = readings;
	}
};

TEST(METADATA, MetadataDisabled)
{
	PLUGIN_INFORMATION *info = plugin_info();
	ConfigCategory *config = new ConfigCategory("metadata", info->config);
	ASSERT_NE(config, (ConfigCategory *)NULL);
	config->setItemsValueFromDefault();
	ASSERT_EQ(config->itemExists("config"), true);
	ASSERT_EQ(config->itemExists("enable"), true);
	config->setValue("config", "{ \"Source\" : \"Camera\"}");
	config->setValue("enable", "false");
	ReadingSet *outReadings;
	void *handle = plugin_init(config, &outReadings, Handler);
	vector<Reading *> *readings = new vector<Reading *>;

	long testValue = 2;
	DatapointValue dpv(testValue);
	Datapoint *value = new Datapoint("test", dpv);
	Reading *in = new Reading("test", value);
	readings->push_back(in);

	ReadingSet *readingSet = new ReadingSet(readings);
	plugin_ingest(handle, (READINGSET *)readingSet);

	vector<Reading *> results = outReadings->getAllReadings();
	ASSERT_EQ(results.size(), 1);
	Reading *out = results[0];
	ASSERT_EQ(out->getDatapointCount(), 1);

	vector<Datapoint *> points = out->getReadingData();
	ASSERT_EQ(points.size(), 1);
}

TEST(METADATA, MetadataAddDataPoints)
{
	PLUGIN_INFORMATION *info = plugin_info();
	ConfigCategory *config = new ConfigCategory("replace", info->config);
	ASSERT_NE(config, (ConfigCategory *)NULL);
	config->setItemsValueFromDefault();
	ASSERT_EQ(config->itemExists("config"), true);
	ASSERT_EQ(config->itemExists("enable"), true);
	config->setValue("config", "{ \"Source\":\"Camera\"}");
	config->setValue("enable", "true");
	ReadingSet *outReadings;
	void *handle = plugin_init(config, &outReadings, Handler);
	vector<Reading *> *readings = new vector<Reading *>;

	long testValue = 2;
	DatapointValue dpv(testValue);
	Datapoint *value = new Datapoint("test", dpv);
	Reading *in = new Reading("test", value);
	readings->push_back(in);

	ReadingSet *readingSet = new ReadingSet(readings);
	plugin_ingest(handle, (READINGSET *)readingSet);

	vector<Reading *> results = outReadings->getAllReadings();
	ASSERT_EQ(results.size(), 1);
	Reading *out = results[0];
	ASSERT_EQ(out->getDatapointCount(), 2);

	vector<Datapoint *> points = out->getReadingData();
	ASSERT_EQ(points.size(), 2);
}

TEST(METADATA, MetadataAddSubs)
{
	PLUGIN_INFORMATION *info = plugin_info();
	ConfigCategory *config = new ConfigCategory("replace", info->config);
	ASSERT_NE(config, (ConfigCategory *)NULL);
	config->setItemsValueFromDefault();
	ASSERT_EQ(config->itemExists("config"), true);
	ASSERT_EQ(config->itemExists("enable"), true);
	config->setValue("config", "{ \"Source\":\"Camera $num$\"}");
	config->setValue("enable", "true");
	ReadingSet *outReadings;
	void *handle = plugin_init(config, &outReadings, Handler);
	vector<Reading *> *readings = new vector<Reading *>;

	long testValue = 2;
	DatapointValue dpv(testValue);
	Datapoint *value = new Datapoint("num", dpv);
	Reading *in = new Reading("test", value);
	readings->push_back(in);

	ReadingSet *readingSet = new ReadingSet(readings);
	plugin_ingest(handle, (READINGSET *)readingSet);

	vector<Reading *> results = outReadings->getAllReadings();
	ASSERT_EQ(results.size(), 1);
	Reading *out = results[0];
	ASSERT_EQ(out->getDatapointCount(), 2);

	vector<Datapoint *> points = out->getReadingData();
	ASSERT_EQ(points.size(), 2);
	Datapoint *dp = out->getDatapoint("Source");
	ASSERT_STREQ(dp->getData().toStringValue().c_str(), "Camera 2");
}

TEST(METADATA, INTEGER_MIN_MAX_LIMITS)
{
	PLUGIN_INFORMATION *info = plugin_info();
	ConfigCategory *config = new ConfigCategory("metadata", info->config);
	ASSERT_NE(config, (ConfigCategory *)NULL);
	config->setItemsValueFromDefault();
	ASSERT_EQ(config->itemExists("config"), true);
	ASSERT_EQ(config->itemExists("enable"), true);
	config->setValue("config", "{ \"INT1\":2147483647, \"INT2\":-2147483647 }");
	config->setValue("enable", "true");
	ReadingSet *outReadings;
	void *handle = plugin_init(config, &outReadings, Handler);
	vector<Reading *> *readings = new vector<Reading *>;

	long testValue = 2;
	DatapointValue dpv(testValue);
	Datapoint *value = new Datapoint("test", dpv);
	Reading *in = new Reading("test", value);
	readings->push_back(in);

	ReadingSet *readingSet = new ReadingSet(readings);
	plugin_ingest(handle, (READINGSET *)readingSet);

	vector<Reading *> results = outReadings->getAllReadings();
	ASSERT_EQ(results.size(), 1);
	Reading *out = results[0];

	vector<Datapoint *> points = out->getReadingData();
	ASSERT_EQ(points.size(), 3);
	
	ASSERT_EQ(points[0]->getName(), "test");
	ASSERT_EQ(points[0]->getData().toInt(), 2);

	ASSERT_EQ(points[1]->getName(), "INT1");
	ASSERT_EQ(points[1]->getData().toInt(), 2147483647);

	ASSERT_EQ(points[2]->getName(), "INT2");
	ASSERT_EQ(points[2]->getData().toInt(), -2147483647);
}

TEST(METADATA, LONG_MIN_MAX_LIMITS)
{
	PLUGIN_INFORMATION *info = plugin_info();
	ConfigCategory *config = new ConfigCategory("metadata", info->config);
	ASSERT_NE(config, (ConfigCategory *)NULL);
	config->setItemsValueFromDefault();
	ASSERT_EQ(config->itemExists("config"), true);
	ASSERT_EQ(config->itemExists("enable"), true);
	config->setValue("config", "{ \"LONG1\":9223372036854775807, \"LONG2\":-9223372036854775807 }");
	config->setValue("enable", "true");
	ReadingSet *outReadings;
	void *handle = plugin_init(config, &outReadings, Handler);
	vector<Reading *> *readings = new vector<Reading *>;

	long testValue = 2;
	DatapointValue dpv(testValue);
	Datapoint *value = new Datapoint("test", dpv);
	Reading *in = new Reading("test", value);
	readings->push_back(in);

	ReadingSet *readingSet = new ReadingSet(readings);
	plugin_ingest(handle, (READINGSET *)readingSet);

	vector<Reading *> results = outReadings->getAllReadings();
	ASSERT_EQ(results.size(), 1);
	Reading *out = results[0];

	vector<Datapoint *> points = out->getReadingData();
	ASSERT_EQ(points.size(), 3);
	
	ASSERT_EQ(points[0]->getName(), "test");
	ASSERT_EQ(points[0]->getData().toInt(), 2);

	ASSERT_EQ(points[1]->getName(), "LONG1");
	ASSERT_EQ(points[1]->getData().toInt(), 9223372036854775807);

	ASSERT_EQ(points[2]->getName(), "LONG2");
	ASSERT_EQ(points[2]->getData().toInt(), -9223372036854775807);
}

TEST(METADATA, MetadataNestedJSON)
{
	PLUGIN_INFORMATION *info = plugin_info();
	ConfigCategory *config = new ConfigCategory("metadata", info->config);
	ASSERT_NE(config, (ConfigCategory *)NULL);
	config->setItemsValueFromDefault();
	ASSERT_EQ(config->itemExists("config"), true);
	ASSERT_EQ(config->itemExists("enable"), true);
	config->setValue("config", "{ \"value\":{\"Building\": \"Pearson\", \"Location\": \"NY\"}}");
	config->setValue("enable", "true");
	ReadingSet *outReadings;
	void *handle = plugin_init(config, &outReadings, Handler);
	vector<Reading *> *readings = new vector<Reading *>;

	long testValue = 2;
	DatapointValue dpv(testValue);
	Datapoint *value = new Datapoint("test", dpv);
	Reading *in = new Reading("test", value);
	readings->push_back(in);

	ReadingSet *readingSet = new ReadingSet(readings);
	plugin_ingest(handle, (READINGSET *)readingSet);

	vector<Reading *> results = outReadings->getAllReadings();
	ASSERT_EQ(results.size(), 1);
	Reading *out = results[0];
	ASSERT_EQ(out->getDatapointCount(), 3);

	vector<Datapoint *> points = out->getReadingData();
	ASSERT_EQ(points.size(), 3);

	ASSERT_EQ(points[0]->getName(), "test");
	ASSERT_EQ(points[0]->getData().toInt(), 2);

	ASSERT_EQ(points[1]->getName(), "Building");
	ASSERT_EQ(points[1]->getData().toStringValue(), "Pearson");

	ASSERT_EQ(points[2]->getName(), "Location");
	ASSERT_EQ(points[2]->getData().toStringValue(), "NY");
}
