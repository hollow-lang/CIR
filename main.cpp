#include <iostream>
#include <fstream>

#include <filesystem>

// NOTE: No need for implementation we will link with .so
// #define CIR_IMPLEMENTATION
#include "core/helpers/cli.h"


int main(int argc, char *argv[]) {
    try {
        ArgParser parser;
        CliConfig config = parser.parse(argc, argv);

        CliTool tool(config);
        return tool.run();
    } catch (const std::exception &e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }
}
