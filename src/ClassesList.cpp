#include "ClassesList.hpp"
#include "RecordExample.hpp"

// Инициализация списка функций
std::vector<InitFunction> initStaticFunctions = 
{
    RecordTest2::initStatic,
    RecordTest::initStatic
};

std::vector<GetRecordClass> GetRecordClassFunctions = 
{
    RecordTest2::getRecordClass,    // 0
    RecordTest::getRecordClass      // 1
};