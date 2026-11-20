#include "strata_tui.h"

#include <iostream>
#include <csignal>

static sandwich::tui::StrataTUI * g_tui = nullptr;

static void sigint_handler(int) {
    if (g_tui) {
        g_tui->shutdown();
    }
    std::exit(0);
}

int main(int argc, char ** argv) {
    (void)argc;
    (void)argv;

    std::signal(SIGINT, sigint_handler);
    std::signal(SIGTERM, sigint_handler);

    sandwich::tui::StrataTUI tui_app;
    g_tui = &tui_app;

    if (!tui_app.init()) {
        std::cerr << "Failed to initialize Strata TUI.\n";
        return 1;
    }

    tui_app.run();
    tui_app.shutdown();

    return 0;
}
