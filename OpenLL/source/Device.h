#ifndef _DEVICE_H_
#define _DEVICE_H_

#pragma once

#include <map>
#include <vector>
#include <string>
using namespace std;

// A Device in OpenLL maintains information about a lighting device.
// This class is meant to hold information about different parameters in
// an OpenLL friendly way. Conversion to network values happens in a
// different class to separate the abstract representation of a device
// from the practical network control details.
class Device
{
public:
  // Default constructor. Every device needs an id, channel, and type.
  // May in the future pull default parameter map from a database of known
  // fixture types.
  Device(string id, unsigned int channel, string type);

  // Destructor. Obviously.
  ~Device();

  // Accessors for id
  string getId() { return id; }
  void setId(string newId) { id = newId; }

  // Accessors for channel
  unsigned int getChannel() { return channel; }
  void setChannel(unsigned int newChan) { channel = newChan; }

  // Accesors for type
  string getType() { return type; }
  void setType(string newType) { type = newType; }

  // Gets a parameter value. Returns false if no parameter with the given name exists.
  // Returns true with the parameter value in val if successful.
  bool getParam(string param, float& val);
  
  // Sets a parameter and returns true if parameter changed does not exist prior to set.
  // Returns false otherwise.
  bool setParam(string param, float val);

  // Resets the values in the parameters to 0 (or equivalent default)
  void clearParamValues();

  // Returns number of parameters in the device.
  unsigned int numParams();

  // Returns list of parameters in the device.
  vector<string> getParamNames();

  // Gets the metadata for a given key. Returns false if no key exists.
  // 
  bool getMetadata(string key, string& val);

  // Sets metadata and returns true if metadata key does not exist prior to set.
  // Returns false otherwise.
  bool setMetadata(string key, string val);

  // Resets metadata values to "" but leaves the keys intact.
  void clearMetadataValues();

  // Empties everything in the metadata hash: keys and values. All gone.
  void clearAllMetadata();

  // Returns the number of metadata keys.
  unsigned int numMetadataKeys();

  // Gets list of metadata keys the device has values for.
  vector<string> getMetadataKeyNames();

private:
  // Unique identifier for the device.
  // Uniqueness isn't quite enforceable at the device level.
  string id;

  // Channel number for the fixture. Does not have to be unique.
  unsigned int channel;

  // Device type name. "Source Four ERS" for example.
  string type;

  // Map for time-varying parameters.
  // Type may change in the future as more specialized datatypes come up.
  map<string, float> parameters;

  // Map for static information.
  // User-defined data helps to add search filters and query devices.
  map<string, string> metadata;
};

#endif