// main.cpp
// Entry point.  Maps to the Perl main() sub.
//
// Perl flow:
//   main()
//     initialize()
//     StartClient::set()
//     OptionsHandler::get()
//     set_debugger()
//     CheckTools::set()
//     set_colors()
//     set_sep()
//     OutputGenerator::generate()
//     cleanup()
//     exit 0

#include "global_state.h"
#include "initialize.h"
#include "configs.h"
#include "check_tools.h"
#include "colors.h"
#include "options_handler.h"
#include "output_generator.h"
#include "start_client.h"

int main(int argc, char* argv[]) {
    inxi::GlobalState state;

    // 1. Bootstrap path and OS detection
    inxi::initialize(state);

    // 2. Detect IRC / terminal client
    inxi::StartClient start_client(state);
    start_client.detect();

    // 3. Parse CLI flags
    inxi::OptionsHandler opts(state);
    opts.parse(argc, argv);

    // 4. Apply debugger level (may have been set by -d flag)
    inxi::Configs configs(state);
    configs.set_debugger();

    // 5. Probe required external tools
    inxi::CheckTools tools(state);
    tools.probe();

    // 6. Apply color scheme
    inxi::set_colors(state);

    // 7. Set separators (IRC vs console)
    inxi::set_sep(state);

    // 8. Run and print all requested info sections
    inxi::OutputGenerator gen(state);
    gen.generate();

    // 9. Flush logs, temp files
    inxi::cleanup(state);

    return 0;
}
