#include "common/logger.h"
#include "common/config_manager.h"
#include <iostream>

using namespace radar::common;

int main()
{
    try
    {
        std::cout << "1. Initializing logger..." << std::endl;
        LoggerConfig logConfig;
        logConfig.console.enabled = true;
        logConfig.file.enabled = false;
        logConfig.globalLevel = LogLevel::INFO;
        LoggerManager::getInstance().initialize(logConfig);
        std::cout << "   Logger initialized successfully" << std::endl;

        std::cout << "2. Testing RADAR_ERROR macro..." << std::endl;
        RADAR_ERROR("Test error message: {}", "test");
        std::cout << "   RADAR_ERROR works" << std::endl;

        std::cout << "3. Getting ConfigManager instance..." << std::endl;
        auto &manager = ConfigManager::getInstance();
        std::cout << "   ConfigManager instance created" << std::endl;

        std::cout << "4. Testing isLoaded()..." << std::endl;
        bool loaded = manager.isLoaded();
        std::cout << "   isLoaded() = " << loaded << std::endl;

        std::cout << "5. All tests passed!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cout << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cout << "Unknown exception caught" << std::endl;
        return 1;
    }

    return 0;
}
