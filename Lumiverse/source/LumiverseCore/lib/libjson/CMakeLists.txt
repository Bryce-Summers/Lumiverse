add_library(libjson
  JSONOptions.h
  _internal/Source/libjson.cpp
  _internal/Source/internalJSONNode.cpp
  _internal/Source/JSONAllocator.cpp
  _internal/Source/JSONChildren.cpp
  _internal/Source/JSONDebug.cpp
  _internal/Source/JSONIterators.cpp
  _internal/Source/JSONMemory.cpp
  _internal/Source/JSONNode.cpp
  _internal/Source/JSONNode_Mutex.cpp
  _internal/Source/JSONPreparse.cpp
  _internal/Source/JSONStream.cpp
  _internal/Source/JSONValidator.cpp
  _internal/Source/JSONWorker.cpp
  _internal/Source/JSONWriter.cpp
  _internal/Source/libjson.cpp
)

INSTALL(TARGETS libjson ARCHIVE DESTINATION lib)
