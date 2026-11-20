#pragma once

#include "sandwich_tui_backend.h"

#include <string>
#include <vector>
#include <atomic>
#include <termios.h>

namespace sandwich {
namespace tui {

enum class TuiScreen {
    SETUP = 0,
    INFERENCE = 1
};

class StrataTUI {
public:
    StrataTUI();
    ~StrataTUI();

    bool init();
    void run();
    void shutdown();

private:
    void setup_terminal();
    void restore_terminal();
    void get_terminal_size(int & rows, int & cols);

    // Screen Renderers
    void render_setup_screen();
    void render_inference_screen();

    // Input Handlers
    void handle_setup_input(int key);
    void handle_inference_input(int key);

    // Drawing Helpers
    void draw_box(int y, int x, int h, int w, const std::string & title, const std::string & border_color);
    void draw_progress_bar(int y, int x, int width, double pct, const std::string & fill_color);

    // State
    struct termios orig_termios_;
    bool terminal_initialized_ = false;
    std::atomic<bool> running_{true};
    TuiScreen current_screen_ = TuiScreen::SETUP;

    // Configuration & Data
    StrataInferenceConfig config_;
    std::vector<std::string> available_models_;
    std::vector<std::string> available_drafters_;
    size_t selected_model_idx_ = 0;
    size_t selected_drafter_idx_ = 0;
    int focused_field_ = 0;

    // Live Telemetry & Output
    StrataTelemetry telemetry_;
    std::string token_output_buffer_;
    std::atomic<bool> inference_running_{false};
    std::atomic<bool> stop_inference_flag_{false};
    std::string status_message_;
};

} // namespace tui
} // namespace sandwich
