#include "Config.h"

namespace catchdb
{

ConfigPtr Config::load(const std::string &configFileName)
{
    // not implemented yet, just for test purpose
    (void) configFileName;
    ConfigPtr config(new Config());
    config->cacheSize = 500;
    config->blockSize = 32;
    config->writeBufferSize = 64;

    return config;
}

} // namespace catchdb
