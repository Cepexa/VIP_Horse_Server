#pragma once

#include <vector>
#include <functional>
#include <string>
#include <memory>

#include "RecordClass.hpp"

// Объявляем список функций инициализации
using InitFunction = std::function<void(std::function<int(std::shared_ptr<RecordClass>, const std::string&)>)>;
extern std::vector<InitFunction> initStaticFunctions;
using GetRecordClass = std::function<std::shared_ptr<RecordClass>()>;
extern std::vector<GetRecordClass> GetRecordClassFunctions;
