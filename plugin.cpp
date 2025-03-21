/*
 * Fledge "metadata" filter plugin.
 *
 * Copyright (c) 2018 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Amandeep Singh Arora, Mark Riddoch
 */

#include <string>
#include <iostream>
#include <plugin_api.h>
#include <config_category.h>
#include <filter.h>
#include <reading_set.h>
#include <version.h>

#define METADATA "\\\"name\\\" : \\\"value\\\""

#define FILTER_NAME "metadata"

#define DEFAULT_CONFIG "{\"plugin\" : { \"description\" : \"Metadata filter plugin\", " \
                       		"\"type\" : \"string\", " \
				"\"default\" : \"" FILTER_NAME "\", \"readonly\" : \"true\" }, " \
			 "\"enable\": {\"description\": \"A switch that can be used to enable or disable execution of " \
					 "the metadata filter.\", " \
				"\"type\": \"boolean\", " \
				"\"displayName\" : \"Enabled\", " \
				"\"default\": \"false\" }, " \
			"\"config\" : {\"description\" : \"Metadata to add to readings.\", " \
				"\"type\" : \"JSON\", " \
				"\"default\": \"{" METADATA "}\", "\
				"\"order\" : \"1\", \"displayName\" : \"Metadata to add\"} }"


using namespace std;

/**
 * The Filter plugin interface
 */
extern "C" {

/**
 * The plugin information structure
 */
static PLUGIN_INFORMATION info = {
		FILTER_NAME,              // Name
		VERSION,                  // Version
		0,                        // Flags
		PLUGIN_TYPE_FILTER,       // Type
		"1.0.0",                  // Interface version
		DEFAULT_CONFIG	          // Default plugin configuration
};

typedef struct
{
	FledgeFilter *handle;
	std::vector<Datapoint *> metadata;
	std::string	configCatName;
} FILTER_INFO;

/**
 * Parse Nested JSON Object
 * @param metadata Reference to vector of metadata
 * @param nsetedJSON Reference to nestedJSON Object
 *
*/
void parseNestedJSON(std::vector<Datapoint *> &metadata, const Value& nsetedJSON)
{
	for (auto &m : nsetedJSON.GetObject())
	{
		if (m.value.IsString())
		{
			DatapointValue dpv(string(m.value.GetString()));
			metadata.push_back(new Datapoint(m.name.GetString(), dpv));
		}
		else if (m.value.IsDouble())
		{
			DatapointValue dpv(m.value.GetDouble());
			metadata.push_back(new Datapoint(m.name.GetString(), dpv));
		}
		else if (m.value.IsInt64())
		{
			DatapointValue dpv((long) m.value.GetInt64());
			metadata.push_back(new Datapoint(m.name.GetString(), dpv));
		}
		else if (m.value.IsObject())
		{
			parseNestedJSON(metadata, m.value);
		}
		else
		{
			Logger::getLogger()->error("Unable to parse value for metadata field '%s', skipping...", m.name.GetString());
		}
	}
}

/**
 * Return the information about this plugin
 */
PLUGIN_INFORMATION *plugin_info()
{
	return &info;
}

/**
 * Initialise the plugin, called to get the plugin handle and setup the
 * output handle that will be passed to the output stream. The output stream
 * is merely a function pointer that is called with the output handle and
 * the new set of readings generated by the plugin.
 *     (*output)(outHandle, readings);
 * Note that the plugin may not call the output stream if the result of
 * the filtering is that no readings are to be sent onwards in the chain.
 * This allows the plugin to discard data or to buffer it for aggregation
 * with data that follows in subsequent calls
 *
 * @param config	The configuration category for the filter
 * @param outHandle	A handle that will be passed to the output stream
 * @param output	The output stream (function pointer) to which data is passed
 * @return		An opaque handle that is used in all subsequent calls to the plugin
 */
PLUGIN_HANDLE plugin_init(ConfigCategory* config,
			  OUTPUT_HANDLE *outHandle,
			  OUTPUT_STREAM output)
{
	FILTER_INFO *info = new FILTER_INFO;
	info->handle = new FledgeFilter(FILTER_NAME, *config, outHandle, output);
	FledgeFilter *filter = info->handle;
	info->configCatName = config->getName();
	
	// Handle filter configuration
	if (filter->getConfig().itemExists("config"))
	{
		Document	document;
		if (document.Parse(filter->getConfig().getValue("config").c_str()).HasParseError())
		{
			Logger::getLogger()->error("Unable to parse metadata filter config: '%s'", filter->getConfig().getValue("config").c_str());
			return NULL;
		}
		Logger::getLogger()->info("Metadata filter config=%s", filter->getConfig().getValue("config").c_str());
		
		for (Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr)
		{
			if (itr->value.IsString())
			{
				DatapointValue dpv(string(itr->value.GetString()));
        			info->metadata.push_back(new Datapoint(itr->name.GetString(), dpv));
			}
			else if (itr->value.IsDouble())
			{
				DatapointValue dpv(itr->value.GetDouble());
        			info->metadata.push_back(new Datapoint(itr->name.GetString(), dpv));
			}
			else if (itr->value.IsInt64())
			{
				DatapointValue dpv((long) itr->value.GetInt64());
        			info->metadata.push_back(new Datapoint(itr->name.GetString(), dpv));
			}
			else if (itr->value.IsObject() && strcmp(itr->name.GetString(), "value") == 0)
			{
				parseNestedJSON(info->metadata, itr->value);
			}
			else
			{
				Logger::getLogger()->error("Unable to parse value for metadata field '%s', skipping...", itr->name.GetString());
			}
		}
	}
	else
	{
		Logger::getLogger()->info("No config provided for metadata filter, will pass-thru' readings as such");
	}
	
	return (PLUGIN_HANDLE)info;
}

/**
 * Ingest a set of readings into the plugin for processing
 *
 * @param handle	The plugin handle returned from plugin_init
 * @param readingSet	The readings to process
 */
void plugin_ingest(PLUGIN_HANDLE *handle,
		   READINGSET *readingSet)
{
	FILTER_INFO *info = (FILTER_INFO *) handle;
	FledgeFilter* filter = info->handle;
	
	if (!filter->isEnabled())
	{
		// Current filter is not active: just pass the readings set
		filter->m_func(filter->m_data, readingSet);
		return;
	}
	
	ReadingSet *origReadingSet = (ReadingSet *) readingSet;

	// Just get all the readings in the readingset
	const vector<Reading *>& readings = origReadingSet->getAllReadings();
	
	// Iterate thru' the readings
	for (auto &elem : readings)
	{
		for (auto &it : info->metadata)
		{
			if (it->getData().getType() == DatapointValue::T_STRING)
			{
				string name = it->getName();
				string value = it->getData().toStringValue();
				string sub = elem->substitute(value);
				DatapointValue dpv(sub);
				elem->addDatapoint(new Datapoint(name, dpv));
			}
			else
			{
				elem->addDatapoint(new Datapoint(*it));
			}
		}
		//AssetTracker::getAssetTracker()->addAssetTrackingTuple(info->configCatName, elem->getAssetName(), string("Filter"));
		AssetTracker *instance =  nullptr;
		instance = AssetTracker::getAssetTracker();
		if (instance != nullptr)
		{
			instance->addAssetTrackingTuple(info->configCatName, elem->getAssetName(), string("Filter"));
		}

		
	}
	
	filter->m_func(filter->m_data, origReadingSet);
}
/*
 * Plugin reconfigure
 */
void plugin_reconfigure(PLUGIN_HANDLE *handle, const string& newConfig)
{
	FILTER_INFO *info = (FILTER_INFO *)handle;
	FledgeFilter* filter = info->handle;

	filter->setConfig(newConfig);
	// Handle filter configuration
	std::vector<Datapoint *> metadata;
	if (filter->getConfig().itemExists("config"))
	{
		Document	document;
		if (document.Parse(filter->getConfig().getValue("config").c_str()).HasParseError())
		{
			Logger::getLogger()->error("Unable to parse metadata filter config: '%s'", filter->getConfig().getValue("config").c_str());
			return;
		}
		Logger::getLogger()->info("Metadata filter config=%s", filter->getConfig().getValue("config").c_str());
		
		for (Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr)
		{
			if (itr->value.IsString())
			{
				DatapointValue dpv(string(itr->value.GetString()));
        			metadata.push_back(new Datapoint(itr->name.GetString(), dpv));
			}
			else if (itr->value.IsDouble())
			{
				DatapointValue dpv(itr->value.GetDouble());
        			metadata.push_back(new Datapoint(itr->name.GetString(), dpv));
			}
			else if (itr->value.IsInt64())
			{
				DatapointValue dpv((long) itr->value.GetInt64());
        			metadata.push_back(new Datapoint(itr->name.GetString(), dpv));
			}
			else if (itr->value.IsObject() && strcmp(itr->name.GetString(), "value") == 0)
			{
				parseNestedJSON(metadata, itr->value);
			}
			else
			{
				Logger::getLogger()->error("Unable to parse value for metadata field '%s', skipping...", itr->name.GetString());
			}
		}
	}
	std::vector<Datapoint *> tmp;
	tmp = info->metadata;
	info->metadata.clear();
	info->metadata = metadata;
	for (const auto &it : tmp)
		delete it;
}


/**
 * Call the shutdown method in the plugin
 */
void plugin_shutdown(PLUGIN_HANDLE *handle)
{
	FILTER_INFO *info = (FILTER_INFO *) handle;
	delete info->handle;
	for (const auto &it : info->metadata)
		delete it;
	delete info;
}

// End of extern "C"
};

