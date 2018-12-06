/*
 * FogLAMP "metadata" filter plugin.
 *
 * Copyright (c) 2018 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Amandeep Singh Arora
 */

#include <string>
#include <iostream>
#include <plugin_api.h>
#include <config_category.h>
#include <filter.h>
#include <reading_set.h>

#define METADATA ""

#define FILTER_NAME "metadata"

#define DEFAULT_CONFIG "{\"plugin\" : { \"description\" : \"Metadata filter plugin\", " \
                       		"\"type\" : \"string\", " \
				"\"default\" : \"" FILTER_NAME "\" }, " \
			 "\"enable\": {\"description\": \"A switch that can be used to enable or disable execution of " \
					 "the metadata filter.\", " \
				"\"type\": \"boolean\", " \
				"\"default\": \"false\" }, " \
			"\"config\" : {\"description\" : \"Metadata filter configuration.\", " \
				"\"type\" : \"JSON\", " \
				"\"default\" : {" METADATA "}} }"

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
		"1.0.0",                  // Version
		0,                        // Flags
		PLUGIN_TYPE_FILTER,       // Type
		"1.0.0",                  // Interface version
		DEFAULT_CONFIG	          // Default plugin configuration
};

typedef struct
{
	FogLampFilter *handle;
	std::vector<Datapoint *> metadata;
} FILTER_INFO;

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
	info->handle = new FogLampFilter(FILTER_NAME, *config, outHandle, output);
	FogLampFilter *filter = info->handle;
	
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
			else if (itr->value.IsNumber())
			{
				DatapointValue dpv((long) itr->value.GetInt());
        			info->metadata.push_back(new Datapoint(itr->name.GetString(), dpv));
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
	FogLampFilter* filter = info->handle;
	
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
			elem->addDatapoint(new Datapoint(*it));
		}
	}
	
	filter->m_func(filter->m_data, origReadingSet);
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
