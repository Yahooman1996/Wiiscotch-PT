#pragma once

#include "../file_system.h"
#include "../json_reader.h"

FileSystem* WiiFileSystem_create(JsonValue* config, const char* gameName);
