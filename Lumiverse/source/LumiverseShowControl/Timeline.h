#ifndef _TIMELINE_H_
#define _TIMELINE_H_

#pragma once

#include "LumiverseCore.h"
#include "Keyframe.h"

namespace Lumiverse {
namespace ShowControl {

/*!
\brief A Timeline is a list of device parameter values at artitrary times

Lumiverse Timelines should be familiar to anyone who's used a timeline based system before.
The one difference to highlight specifically is the addition of a new keyframe data type called
"Use Current State." When this keyframe value is present, the state of the layer the timeline is
being played on will be used as the "current state" whenever this keyframe is encountered. Because
of this, this keyframe is typically used only at the beginning of timelines.
*/
class Timeline {
public:
  Timeline();

  ~Timeline();

  /*!
  \brief Gets the identifier used to refer to a device-parameter keyframe set.
  */
  string getTimelineKey(Device* d, string paramName);

  /*!
  \brief Gets the keyframes for a given identifier and time. Read-only.
  */
  Keyframe getKeyframe(string identifier, size_t time);

  /*!
  \brief Gets the keyframes for a given device and time.
  */
  map<string, Keyframe*> getKeyframes(Device* d, size_t time);

  /*!
  \brief Gets the keyframes for the entire timeline.
  */
  map<string, set<Keyframe> >& const getAllKeyframes();

  /*!
  \brief Sets the value for the specified keyframe.

  This function will overwrite existing keyframes without warning.
  */
  void setKeyframe(string identifier, size_t time, LumiverseType* data);

  /*!
  \brief Sets the value of the keyframes for all parameters of the given device.
  */
  void setKeyframe(Device* d, size_t time);

  /*!
  \brief Stores a keyframe from a rig
  */
  void setKeyframes(Rig* rig, size_t time);

  /*!
  \brief Stores a keyframe for a group of selected devices
  */
  void setKeyframes(DeviceSet devices, size_t time);

  /*!
  \brief Deletes the keyframe with the specified identifier at the specified time.
  */
  void deleteKeyframe(string identifier, size_t time);

  /*!
  \brief Deletes the keyframes for the specified device at the specified time.
  */
  void deleteKeyframe(Device* d, size_t time);

  /*!
  \brief Deletes the keyframes for the specified devices at the specified time.
  */
  void deleteKeyframe(DeviceSet devices, size_t time);

  /*!
  \brief Returns the value of the specified parameter for the specified device at the specified time.

  \param identifier [deviceID]:[paramName] for the desired device and parameter
  \param time Time in microseconds to get the value.
  \return A LumiverseType value for the specified time in the timeline.
  */
  shared_ptr<LumiverseType*> getValueAtTime(string identifier, size_t time);

  /*!
  \brief Gets the length of the timeline based on stored keyframes.
  */
  size_t getLength();

private:
  /*!
  \brief Stores the length of the timeline.

  The length is calculated as needed, as it is a potentially time consuming thing
  to figure out.
  */
  size_t _length;

  /*!
  \brief Indicates if the timeline's length is updated.
  */
  bool _lengthIsUpdated;

  /*!
  \brief Map from unique identifier to timeline keyframes.

  The unique identifier for device parameter pair is: [deviceID]:[paramName]
  */
  map<string, set<Keyframe> > _timelineData;
};

}
}
#endif