#pragma once

#include "sandwich_tui_backend.h"

#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <termios.h>

namespace sandwich {
namespace tui {

enum class TuiScreen {
    SETUP = 0,
    CHAT = 1,
    CONNECTING = 2,
    SHUTDOWN_CONFIRM = 3
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
    void render_chat_screen();
    void render_connecting_screen();
    void render_shutdown_confirm_screen();

    // Input Handlers
    void handle_setup_input(int key);
    void handle_chat_input(int key);
    void handle_connecting_input(int key);
    void handle_shutdown_confirm_input(int key);

    // Core Actions
    void trigger_model_switch(size_t new_idx);
    void send_current_message();
    void abort_active_generation();
    void copy_cli_command();

    // Drawing Helpers
    void draw_box(int y, int x, int h, int w, const std::string & title, const std::string & border_color);
    void draw_progress_bar(int y, int x, int width, double pct, const std::string & fill_color);

    // Terminal & App State
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

    // Server Process Lifecycle
    pid_t spawned_server_pid_ = -1;
    bool server_healthy_ = false;
    std::string server_status_text_;
    int connecting_spinner_idx_ = 0;
    std::chrono::steady_clock::time_point connect_start_time_;

    // Live Telemetry & HUD
    StrataTelemetry telemetry_;
    bool hud_visible_ = true;
    std::chrono::steady_clock::time_point last_telemetry_poll_;

    // Chat Interface
    std::vector<StrataChatMessage> chat_history_;
    std::string input_buffer_;
    size_t input_cursor_ = 0;
    int scroll_offset_ = 0;
    bool auto_scroll_ = true;
    std::atomic<bool> inference_streaming_{false};
    std::atomic<bool> abort_inference_flag_{false};
    std::string active_streaming_text_;

    // Notifications / Toasts
    std::string toast_message_;
    std::chrono::steady_clock::time_point toast_expire_;
};

} // namespace tui
} // namespace sandwich
