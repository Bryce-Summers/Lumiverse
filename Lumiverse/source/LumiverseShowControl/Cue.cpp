#include "Cue.h"
#ifndef _CUE_H_

namespace Lumiverse {
namespace ShowControl {

Keyframe::Keyframe(JSONNode node) {
  auto it = node.begin();
  while (it != node.end()) {
    string name = it->name();

    if (name == "time")
      t = it->as_float();
    else if (name == "usePreviousValue")
      usePreviousValue = it->as_bool();
    else if (name == "val") {
      if (it->type() == 's') {
        val = nullptr;
      }
      else {
        val = shared_ptr<LumiverseType>(LumiverseTypeUtils::loadFromJSON(*it));
      }
    }

    it++;
  }
}

Cue::Cue(Rig* rig) : m_upfade(3.0f), m_downfade(3.0f), m_delay(0) {
  update(rig);
}

Cue::Cue(Rig* rig, float time) : m_upfade(time), m_downfade(time), m_delay(0) {
  update(rig);
}

Cue::Cue(map<string, Device*> devices, float up, float down, float delay)
  : m_upfade(up), m_downfade(down), m_delay(delay) {
  update(devices);
}

Cue::Cue(Rig* rig, float up, float down) : m_upfade(up), m_downfade(down), m_delay(0) {
  update(rig);
}

Cue::Cue(Rig* rig, float up, float down, float delay) : m_upfade(up), m_downfade(down), m_delay(delay) {
  update(rig);
}

Cue::Cue(JSONNode node) {
  auto it = node.begin();
  while (it != node.end()) {
    string name = it->name();

    if (name == "upfade")
      m_upfade = it->as_float();
    else if (name == "downfade")
      m_downfade = it->as_float();
    else if (name == "delay")
      m_delay = it->as_float();
    else if (name == "cueData") {
      // The big one. Need to load the giant cueData map.
      // devices are the top level
      auto device = it->begin();
      while (device != it->end()) {
        string deviceName = device->name();
        
        // Parameter level
        auto param = device->begin();
        while (param != device->end()) {
          string paramName = param->name();

          // Get the kayframes.
          auto kf = param->begin();
          while (kf != param->end()) {
            m_cueData[deviceName][paramName].insert(Keyframe(*kf));
            kf++;
          }
        
          param++;
        }
        device++;
      }
    }

    it++;
  }

  m_lengthIsUpdated = false;
  m_type = INVALID;
}

Cue::Cue(const Cue& other) {
  m_upfade = other.m_upfade;
  m_downfade = other.m_downfade;
  m_delay = other.m_delay;
  m_length = other.m_length;
  m_lengthIsUpdated = other.m_lengthIsUpdated;
  m_type = other.m_type;

  // Fully copy over cue data. Other instance may go out of scope whenever and
  // delete the cue data but since we have a shared_ptr, we should still have it.
  for (auto& it : other.m_cueData) {
    for (auto& param : it.second) {
      m_cueData[it.first][param.first] = param.second;
    }
  }
}

Cue::~Cue() {
  // shared_ptr should keep an internal count of references and delete itself
  // after the container is destroyed.
}

void Cue::operator=(const Cue& other) {
  m_upfade = other.m_upfade;
  m_downfade = other.m_downfade;
  m_delay = other.m_delay;
  m_length = other.m_length;
  m_lengthIsUpdated = other.m_lengthIsUpdated;
  m_type = other.m_type;

  // Fully copy over cue data. Other instance may go out of scope whenever and
  // delete the cue data but since we have a shared_ptr, we should still have it.
  for (auto& it : other.m_cueData) {
    for (auto &param : it.second) {
      m_cueData[it.first][param.first] = param.second;
    }
  }
}

Cue::changedParams Cue::update(Rig* rig) {
  changedParams params;
  m_lengthIsUpdated = false;
  m_type = INVALID;
  
  for (auto d : rig->getDeviceRaw()) {
    if (m_cueData.count(d->getId()) == 0) {
      // New cues don't send back changed parameters since there weren't really
      // things to change before they got added.
      // In a timeline system, this first update sets the initial state
      m_cueData[d->getId()] = getParams(d);
    }
    else {
      map<string, shared_ptr<Lumiverse::LumiverseType> > changed;
      updateParams(d, changed);

      if (changed.size() > 0) {
        params[d->getId()] = changed;
      }
    }
  }

  return params;
}

Cue::changedParams Cue::update(map<string, Device*> devices) {
  changedParams params;
  m_lengthIsUpdated = false;
  m_type = INVALID;

  for (auto kvp : devices) {
    if (m_cueData.count(kvp.second->getId()) == 0) {
      // New cues don't send back changed parameters since there weren't really
      // things to change before they got added.
      // In a timeline system, this first update sets the initial state and the ending keyframes
      // based on the timing provided in the beginning.
      m_cueData[kvp.second->getId()] = getParams(kvp.second);
    }
    else {
      // Note that this function only checks the first keyframe in the cue, so if you
      // have a more complicated set up this is much less useful
      map<string, shared_ptr<Lumiverse::LumiverseType> > changed;
      updateParams(kvp.second, changed);

      if (changed.size() > 0) {
        params[kvp.second->getId()] = changed;
      }
    }
  }

  return params;
}

void Cue::trackedUpdate(Cue::changedParams& oldVals, Rig* rig) {
  auto it = oldVals.begin();
  m_lengthIsUpdated = false;
  m_type = INVALID;

  while (it != oldVals.end()) {
    auto params = it->second.begin();

    // Tracking individual keyframe values is a bit more involved as we have to check each keyframe except
    // the ones with val set to nullptr. 
    // For each parameter...
    while (params != it->second.end()) {
      // For each keyframe associated with that parameter in the cue...
      for (auto keyframe : m_cueData[it->first][params->first]) {
        // Theoretically this should be the last thing in a list of keyframes for a parameter
        if (keyframe.val == nullptr)
          continue;

        // If the param is the same as in oldVal, update it in the keyframe
        if (LumiverseTypeUtils::equals(keyframe.val.get(), params->second.get())) {
          LumiverseTypeUtils::copyByVal(rig->getDevice(it->first)->getParam(params->first), keyframe.val.get());
          ++params;
        }
        // Otherwise stop.
        else {
          oldVals[it->first].erase(params++);
        }
      }
    }

    // If a device has no changes to track, delete it from the tracking list.
    if (oldVals[it->first].size() == 0) {
      oldVals.erase(it++);
    }
    else {
      ++it;
    }
  }
}

void Cue::setDelay(float delay) {
  setTime(m_upfade, m_downfade, delay);
}

void Cue::setTime(float time) {
  setTime(time, time, m_delay);
}

void Cue::setTime(float up, float down) {
  setTime(up, down, m_delay);
}

void Cue::setTime(float up, float down, float delay) {
  m_upfade = up;
  m_downfade = down;
  m_delay = delay;
  m_lengthIsUpdated = false;
}

void Cue::insertKeyframe(string id, string param, Lumiverse::LumiverseType* data, float time, bool upv) {
  Keyframe end = *prev(m_cueData[id][param].end());

  Keyframe k(time, shared_ptr<Lumiverse::LumiverseType>(LumiverseTypeUtils::copy(data)), false);

  // If the user wants to use the previous cue value in the transition, we do that here.
  if (time >= end.t)
    k.usePreviousValue = upv;

  auto old = m_cueData[id][param].insert(k);

  // If a keyframe already existed, delete it an reinsert.
  if (!old.second) {
    m_cueData[id][param].erase(old.first);
    m_cueData[id][param].insert(k);
  }

  m_lengthIsUpdated = false;
  m_type = INVALID;
}

void Cue::insertKeyframe(float time, DeviceSet devices, bool uct) {
  // For each device
  for (auto d : devices.getDevices()) {
    // For each parameter
    for (auto p : d->getRawParameters()) {
      insertKeyframe(d->getId(), p.first, d->getParam(p.first), time, uct);
    }
  }

  m_lengthIsUpdated = false;
  m_type = INVALID;
}

void Cue::deleteKeyframe(string id, string param, float time) {
  // don't try to erase something that's not there
  if (m_cueData.count(id) == 0 || m_cueData[id].count(param) == 0)
    return;
  
  // Find the cue data entry at the given time.
  for (auto it = m_cueData[id][param].begin(); it != m_cueData[id][param].end(); it++) {
    if (it->t == time) {
      m_cueData[id][param].erase(it);
      return;
    }
  }

  m_lengthIsUpdated = false;
  m_type = INVALID;
}

void Cue::deleteKeyframe(float time, DeviceSet devices) {
  // For each device
  for (auto& d : devices.getDevices()) {
    // For each parameter
    for (auto& p : d->getRawParameters()) {
      deleteKeyframe(d->getId(), p.first, time);
    }
  }

  m_lengthIsUpdated = false;
  m_type = INVALID;
}

map<string, set<Keyframe> > Cue::getParams(Device* d) {
  map<string, set<Keyframe> > paramKeyframes;

  // Copy all parameters to cue data list
  for (auto& a : d->getRawParameters()) {
    // Insert the starting point as a keyframe.
    paramKeyframes[a.first].insert(Keyframe(0, shared_ptr<Lumiverse::LumiverseType>(LumiverseTypeUtils::copy(a.second)), false));
  }

  return paramKeyframes;
}

void Cue::updateParams(Device* d, map<string, shared_ptr<Lumiverse::LumiverseType> >& changed) {
  for (auto p : m_cueData[d->getId()]) {
    for (auto k : p.second) {
      // If parameter data is new, replace this cue's data with the new data.
      if (!LumiverseTypeUtils::equals(d->getParam(p.first), k.val.get())) {
        // Make a full copy of the old data before overwriting old data.
        changed[p.first] = shared_ptr<Lumiverse::LumiverseType>(LumiverseTypeUtils::copy(k.val.get()));

        LumiverseTypeUtils::copyByVal(d->getParam(p.first), k.val.get());
      }
    }
  }
}

JSONNode Cue::toJSON() {
  JSONNode cue;
  cue.push_back(JSONNode("upfade", m_upfade));
  cue.push_back(JSONNode("downfade", m_downfade));
  cue.push_back(JSONNode("delay", m_delay));

  JSONNode cueData;

  // Get the actual data.
  for (auto& id : m_cueData) {
    // Device
    JSONNode deviceData;
    deviceData.set_name(id.first);
    
    for (auto& param : id.second) {
      // Parameter
      JSONNode paramData;
      paramData.set_name(param.first);

      for (auto& keyframe : param.second) {
        // Keyframes
        JSONNode kf;
        stringstream ss;
        ss << keyframe.t;

        kf.set_name(ss.str());
        kf.push_back(JSONNode("time", keyframe.t));

        if (keyframe.val != nullptr) {
          kf.push_back(keyframe.val->toJSON("val"));
        }
        else {
          kf.push_back(JSONNode("val", "null"));
        }
        kf.push_back(JSONNode("usePreviousValue", keyframe.usePreviousValue));

        paramData.push_back(kf);
      }

      deviceData.push_back(paramData);
    }
    cueData.push_back(deviceData);
  }

  cueData.set_name("cueData");
  cue.push_back(cueData);

  return cue;
}

float Cue::getLength() {
  if (m_lengthIsUpdated) {
    return m_length;
  }
  else {
    // the hard way.
    // Go through and find the maximum time that a keyframe is set to.
    // First figure out if we should count upfade or downfade if a keyframe has useCueTiming set to true.
    float cueTiming = (m_upfade > m_downfade) ? m_upfade : m_downfade;
    float time = 0;

    for (const auto& id : m_cueData) {
      for (const auto& param : id.second) {
        // Get the last keyframe, this is sorted.
        auto lastKeyframe = param.second.rbegin();
        
        // Time is equal to transition time + largest keyframe time.
        time = (lastKeyframe->t > time) ? lastKeyframe->t : time;
      }
    }

    // Lenght is the cue time + delay.
    m_length = time + m_delay;
    
    // Cache it.
    m_lengthIsUpdated = true;
    return m_length;
  }
}

Cue::Type Cue::getType() {
  if (m_type == INVALID)
  {
    bool allAtTimeZero = true;
    bool noneUsePreviousData = true;
    bool allUsePreviousData = true;

    for (const auto& id : m_cueData) {
      for (const auto& param : id.second) {
        // Check first keyframes.
        allAtTimeZero &= (param.second.begin()->t == 0) && param.second.size() == 1;
        noneUsePreviousData &= !(param.second.begin()->usePreviousValue);
        allUsePreviousData &= param.second.begin()->usePreviousValue;
      }
    }
    float t = getTransitionTime();

    if (allAtTimeZero && noneUsePreviousData)
      m_type = SCENE;
    else if (!allAtTimeZero && noneUsePreviousData && t == 0)
      m_type = STANDALONE;
    else if (!allAtTimeZero && noneUsePreviousData && t != 0)
      m_type = HYBRID;
    else if (allAtTimeZero && allUsePreviousData && t == 0)
      m_type = LINKED;
    else if (allAtTimeZero && !noneUsePreviousData && t == 0)
      m_type = PARTIAL_LINK;
    else
      m_type = OTHER;
  }

  return m_type;
}

Keyframe Cue::getLastKeyframe(string device, string param) {
  return Keyframe(*m_cueData[device][param].rbegin());
}

Keyframe Cue::getFirstKeyframe(string device, string param) {
  return Keyframe(*m_cueData[device][param].begin());
}

shared_ptr<LumiverseType> Cue::getValueAtTime(Cue* previousCue, string device, string param, float t)
{
  // Determine if we're using an upfade or downfade.
  Keyframe prevKeyframe;
  
  if (previousCue != nullptr)
    prevKeyframe = previousCue->getLastKeyframe(device, param);

  Keyframe firstKeyframe = getFirstKeyframe(device, param);
  
  float fadeTime = (LumiverseTypeUtils::cmp(prevKeyframe.val.get(), firstKeyframe.val.get()) == -1) ? m_upfade : m_downfade;
  fadeTime += m_delay;
  
  // Still in the transition
  if (t < fadeTime) {
    // If we're not done delaying, return the previous value.
    // Also, if we're set to use the previous value for the first keyframe, also automatically return the previous value.
    if (previousCue != nullptr && (t < m_delay || firstKeyframe.usePreviousValue))
      return prevKeyframe.val;
    else if (previousCue == nullptr) {
      // If we don't have a previous cue for some reason, we return the first keyframe in the target cue.
      Logger::log(ERR, "Attempting to delay a transition with no previous cue specified.");
      return firstKeyframe.val;
    }
    
    // If we are done, do a lerp between previous and next keyframe.
    float a = (t - m_delay) / (fadeTime - m_delay);
    return LumiverseTypeUtils::lerp(prevKeyframe.val.get(), firstKeyframe.val.get(), a);
  }
  else {
    // If we're out of the transition, it's the same as requesting the value at cue time (t - fade)
    t -= fadeTime;
    return getValueAtCueTime(previousCue, device, param, t);
  }
}
  
shared_ptr<LumiverseType> Cue::getValueAtCueTime(Cue* previousCue, string device, string param, float t) {
  // get the keyframes if they exist, otherwise return null immediately.
  if (m_cueData[device].count(param) == 0)
    return nullptr;
  
  auto keyframes = m_cueData[device][param];
  Keyframe first;
  Keyframe next;
  bool nextFound = false;

  for (auto keyframe = keyframes.begin(); keyframe != keyframes.end();) {
    if (keyframe->t > t) {
      next = *keyframe;
      first = *prev(keyframe);
      nextFound = true;
      break;
    }

    ++keyframe;
  }

  if (!nextFound) {
    // We are at the end of the defined keyframes, so return the value of the most
    // recent keyframe

    // Check to make sure the keyframe has a value
    if (keyframes.rbegin()->usePreviousValue && previousCue != nullptr)
      return previousCue->getLastKeyframe(device, param).val;
    else 
      return keyframes.rbegin()->val;
  }

  // check keyframes for missing values
  if (first.usePreviousValue && previousCue != nullptr)
    first.val = previousCue->getLastKeyframe(device, param).val;

  // If both of these use the previous value it seems a bit silly, but you never know.
  if (next.usePreviousValue && previousCue != nullptr)
    next.val = previousCue->getLastKeyframe(device, param).val;

  // Otherwise we have our keyframes and can now do some ops.
  float a = (t - first.t) / (next.t - first.t);
  return LumiverseTypeUtils::lerp(first.val.get(), next.val.get(), a);
}

bool Cue::paramIsActive(Cue* previousCue, string device, string param, float t) {
  // There are a few cases for this
  // TODO: There is another case where the last keyframe is a function-driven key frame.
  // Since function keyframes aren't added yet, this function is pretty simple.

  // If previousCue is nullptr, we're going to assume they're talking about a timeline
  // based cue, which does not take into account any sort of delay or upfade timings.
  if (previousCue == nullptr) {
    return t <= getLastKeyframe(device, param).t;
  }
  else {
    // Otherwise we do take into account the transition time for the parameter.
    Keyframe prevKeyframe = previousCue->getLastKeyframe(device, param);
    Keyframe firstKeyframe = getFirstKeyframe(device, param);

    float fadeTime = (LumiverseTypeUtils::cmp(prevKeyframe.val.get(), firstKeyframe.val.get()) == -1) ? m_upfade : m_downfade;
    fadeTime += m_delay;

    return (t <= getLastKeyframe(device, param).t + fadeTime);
  }
}

}
}

#endif