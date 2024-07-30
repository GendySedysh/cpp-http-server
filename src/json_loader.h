#ifndef __JSON_LOADER__
#define __JSON_LOADER__

#define BOOST_BEAST_USE_STD_STRING_VIEW

#pragma once

#include <boost/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "model.h"

namespace json_loader {

	model::Game LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader

#endif