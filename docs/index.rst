.. Images
.. |metadata| image:: images/metadata.jpg

Metadata Filter
===============

The *fledge-filter-metadata* filter allows data to be added to assets within Fledge. Metadata takes the form of fixed data points that are added to an asset used to add context to the data. Examples of metadata might be unit of measurement information, location information or identifiers for the piece of equipment to which the measurement relates.

A metadata filter may be added to either a south service or a north task. In a south service it will be adding data for just those assets that originate in that service, in which case it probably relates to a single machine that is being monitored and would add metadata related to that machine. In a north task it causes metadata to be added to all assets that the Fledge is sending to the up stream system, in which case the metadata would probably related to that particular Fledge instance. Adding metadata in the north is particularly useful when a hierarchy of Fledge systems is used and an audit trail is required with the data or the individual Fledge systems related to some physical location information such s building, floor and/or site.

To add a metadata filter 

  - Click on the Applications add icon for your service or task.

  - Select the *metadata* plugin from the list of available plugins.

  - Name your metadata filter.

  - Click *Next* and you will be presented with the following configuration page

+------------+
| |metadata| |
+------------+

  - Enter your metadata in the JSON array shown. You may add multiple items in a single filter by separating them with commas. Each item takes the format of a JSON key/value pair and will be added as data points within the asset.

  - Enable the filter and click on *Done* to activate it

Macro Substitution
------------------

The metadata values may use macro substitution to include the values of other datapoints in the new metadata or the name of the asset. Macros are introduced by the use of $ characters and surround the name of the datapoint to substitute into the metadata.

Let us assume we have a datapoint called *unit* which is the number of the camera attached to our system and we want to have a new datapoint called source which is a string and contains the word camera and the unit number of the camera. We can create metadata of the form

.. code-block:: JSON

        { "source" : "camera $unit$" }

If we wanted to use the asset name of the reading in the metadata, we could use the reserve macro *$ASSET$*

.. code-block:: JSON

        { "source" : "$ASSET$ $unit$" }

This example also illustrates that we can use multiple substitutions in a single meta data item.

If the datapoint that is named in a macro substitution does not exist, then a blank is replaced with the macro name. It is however possible to provide a default value if the datapoint does not exist.

.. code-block:: JSON

        { "source" : "$ASSET$ $unit|unknown$" }

In this case we would substitute the value of the unit datapoint if one existed or the string *unknown* if one did not.

We can use substitution to duplicate a datapoint. Assume we have a datapoint called *location* that we want to both reserve the original value of but also do some destruction processing of later in the data pipeline. In this example we will use another filter to extract the first portion of the location, lets assume this gives us country information. We duplicate the *location* datapoint into a *country* datapoint using the meta filter with the following configuration.


.. code-block:: JSON

        { "country" : "$location$" }

We now have a datapoint called *country* that contains the full location. A later filter in the pipeline will edit the value of this datapoint such that it just contains the country.

Example Metadata
----------------

Assume we are reading the temperature of air entering a paint booth. We might want to add the location of the paint booth, the booth number, the location of the sensor in the booth and the unit of measurement. We would add the following configuration value

.. code-block:: JSON

  {
    "value": {
        "floor": "Third",
        "booth": 1,
        "units": "C",
        "location": "AirIntake"
        }
  }

In above example the filter would add "floor", "booth", "units" and "location" data points to all the readings processed by it. Given an input to the filter of

.. code-block:: JSON

        { "temperature" : 23.4 }

The resultant reading that would be passed onward would become

.. code-block:: JSON

        { "temperature" : 23.5, "booth" : 1, "units" : "C", "floor" : "Third", "location" : "AirIntake" }

This is an example of how metadata might be added in a south service. Turning to the north now, assume we have a configuration whereby we have several sites in an organization and each site has several building. We want to monitor data about the buildings and install a Fledge instance in each building to collect building data. We also install a Fledge instance in each site to collect the data from each individual Fledge instance per building, this allows us to then send the site data to the head office without having to allow each building Fledge to have access to the corporate network. Only the site Fledge needs that access. We want to label the data to say which building it came from and also which site. We can do this by adding metadata at each stage.

To the north task of a building Fledge, for example the "Pearson" building, we add the following metadata

.. code-block:: JSON

   {
     "value" : {
         "building": "Pearson"
         }
   }

Likewise to the "Lawrence" building Fledge instance we add the following to the north task

.. code-block:: JSON

   {
     "value" : {
         "building": "Lawrence"
         }
   }

These buildings are both in the "London" site and will send their data to the site Fledge instance. In this instance we have a north task that sends the data to the corporate headquarters, in this north task we add

.. code-block:: JSON

   {
     "value" : {
         "site": "London"
         }
   }

If we assume we measure the power flow into each building in terms of current, and for the Pearson building we have a value of 117A at 11:02:15 and for the Lawrence building we have a value of 71.4A at 11:02:23, when the data is received at the corporate system we would see readings of

.. code-block:: JSON

   { "current" : 117, "site" : "London", "building" : "Pearson" }
   { "current" : 71.4, "site" : "London", "building" : "Lawrence" }

By adding the data like this it gives as more flexibility, if for example we want to change the way site names are reported, or we acquire a second site in London, we only have to change the metadata in one place.

