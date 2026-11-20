#include "strata_tui.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace sandwich {
namespace tui {

// Color Tokens (Strata Dark-Slate Theme)
static const char * CLR_RESET      = "\033[0m";
static const char * CLR_BOLD       = "\033[1m";
static const char * CLR_DIM        = "\033[2m";
static const char * CLR_TEAL       = "\033[38;2;20;184;166m";
static const char * CLR_CYAN       = "\033[38;2;6;182;212m";
static const char * CLR_EMERALD    = "\033[38;2;16;185;129m";
static const char * CLR_AMBER      = "\033[38;2;245;158;11m";
static const char * CLR_ROSE       = "\033[38;2;244;63;94m";
static const char * CLR_SLATE_FG   = "\033[38;2;203;213;225m";
static const char * CLR_MUTED      = "\033[38;2;100;116;139m";
static const char * CLR_BORDER     = "\033[38;2;51;65;85m";
static const char * CLR_CARD_BG    = "\033[48;2;30;41;59m";
static const char * CLR_HL_BG      = "\033[48;2;51;65;85m";

StrataTUI::StrataTUI() {
    available_models_   = strata_discover_models();
    available_drafters_ = strata_discover_drafters();
    if (!available_models_.empty()) {
        config_.model_path = available_models_[0];
    }
    if (!available_drafters_.empty()) {
        config_.drafter_path = available_drafters_[0];
    }
}

StrataTUI::~StrataTUI() {
    shutdown();
}

bool StrataTUI::init() {
    setup_terminal();
    status_message_ = "Strata TUI initialized. Ready for auto-dispatched inference.";
    return true;
}

void StrataTUI::shutdown() {
    stop_inference_flag_.store(true);
    restore_terminal();
}

void StrataTUI::setup_terminal() {
    if (terminal_initialized_) return;

    tcgetattr(STDIN_FILENO, &orig_termios_);
    struct termios raw = orig_termios_;

    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_oflag &= ~(OPOST);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 1; // 100ms timeout

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    // Hide cursor & use alternate screen buffer
    std::cout << "\033[?1049h\033[?25l" << std::flush;
    terminal_initialized_ = true;
}

void StrataTUI::restore_terminal() {
    if (!terminal_initialized_) return;

    // Show cursor & restore main screen buffer
    std::cout << "\033[?25h\033[?1049l" << std::flush;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios_);
    terminal_initialized_ = false;
}

void StrataTUI::get_terminal_size(int & rows, int & cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        rows = ws.ws_row;
        cols = ws.ws_col;
    } else {
        rows = 24;
        cols = 80;
    }
}

void StrataTUI::draw_box(int y, int x, int h, int w, const std::string & title, const std::string & border_color) {
    if (w < 4 || h < 2) return;

    // Top border
    std::cout << "\033[" << y << ";" << x << "H" << border_color << "┌";
    int title_len = 0;
    if (!title.empty()) {
        std::cout << " " << CLR_BOLD << title << CLR_RESET << border_color << " ";
        title_len = static_cast<int>(title.length()) + 2;
    }
    for (int i = 1 + title_len; i < w - 1; ++i) {
        std::cout << "─";
    }
    std::cout << "┐" << CLR_RESET;

    // Side borders
    for (int i = 1; i < h - 1; ++i) {
        std::cout << "\033[" << (y + i) << ";" << x << "H" << border_color << "│" << CLR_RESET;
        std::cout << "\033[" << (y + i) << ";" << (x + w - 1) << "H" << border_color << "│" << CLR_RESET;
    }

    // Bottom border
    std::cout << "\033[" << (y + h - 1) << ";" << x << "H" << border_color << "└";
    for (int i = 1; i < w - 1; ++i) {
        std::cout << "─";
    }
    std::cout << "┘" << CLR_RESET;
}

void StrataTUI::draw_progress_bar(int y, int x, int width, double pct, const std::string & fill_color) {
    if (width < 4) return;
    pct = std::clamp(pct, 0.0, 100.0);
    int fill_width = static_cast<int>((width * pct) / 100.0);

    std::cout << "\033[" << y << ";" << x << "H" << CLR_MUTED << "[" << fill_color;
    for (int i = 0; i < fill_width; ++i) {
        std::cout << "█";
    }
    std::cout << CLR_BORDER;
    for (int i = fill_width; i < width; ++i) {
        std::cout << "░";
    }
    std::cout << CLR_MUTED << "]" << CLR_RESET;
}

void StrataTUI::render_setup_screen() {
    int rows = 0, cols = 0;
    get_terminal_size(rows, cols);

    std::cout << "\033[2J\033[H"; // Clear screen

    // Top Header Banner
    std::cout << "\033[1;1H" << CLR_CARD_BG << CLR_BOLD << CLR_TEAL
              << " 🥪 SANDWICH ENGINE ── STRATA TUI "
              << CLR_MUTED << "│ "
              << CLR_SLATE_FG << "Host: HP Victus 15 | i7-13620H (6P+4E) | 24GB DDR5 | RTX 5050 Mobile 8GB"
              << CLR_RESET;

    // Model Selection Panel
    int box_w = std::min(cols - 4, 100);
    int start_x = 2;
    int model_box_h = 8;
    draw_box(3, start_x, model_box_h, box_w, "SELECT TARGET MODEL", CLR_TEAL);

    for (size_t i = 0; i < std::min(available_models_.size(), size_t(5)); ++i) {
        bool selected = (i == selected_model_idx_);
        std::cout << "\033[" << (4 + i) << ";" << (start_x + 2) << "H";
        if (selected) {
            std::cout << CLR_AMBER << CLR_BOLD << "▶ " << CLR_HL_BG << CLR_SLATE_FG;
        } else {
            std::cout << "  " << CLR_MUTED;
        }

        std::string filename = fs::path(available_models_[i]).filename().string();
        if (filename.length() > size_t(box_w - 8)) {
            filename = filename.substr(0, box_w - 11) + "...";
        }
        std::cout << filename << CLR_RESET;
    }

    // Mode Configuration Panel
    int config_box_y = 3 + model_box_h + 1;
    int config_box_h = 10;
    draw_box(config_box_y, start_x, config_box_h, box_w, "INFERENCE DISPATCH CONFIGURATION", CLR_CYAN);

    // Mode Toggle
    std::cout << "\033[" << (config_box_y + 1) << ";" << (start_x + 2) << "H";
    std::cout << CLR_BOLD << "Execution Mode: " << CLR_RESET;
    if (config_.auto_mode) {
        std::cout << CLR_EMERALD << CLR_BOLD << "[●] AUTO-DISPATCH (Recommended)" << CLR_RESET
                  << "    " << CLR_MUTED << "[○] Manual Mode" << CLR_RESET;
    } else {
        std::cout << CLR_MUTED << "[○] Auto-Dispatch" << CLR_RESET
                  << "    " << CLR_AMBER << CLR_BOLD << "[●] MANUAL MODE (Power User)" << CLR_RESET;
    }

    // Parameters display
    if (config_.auto_mode) {
        std::cout << "\033[" << (config_box_y + 3) << ";" << (start_x + 2) << "H"
                  << CLR_SLATE_FG << "• Context Window:   " << CLR_TEAL << config_.n_ctx << " tokens" << CLR_RESET;
        std::cout << "\033[" << (config_box_y + 4) << ";" << (start_x + 2) << "H"
                  << CLR_SLATE_FG << "• Compute Threads:  " << CLR_TEAL << config_.n_threads << " (Pinned Physical P-Cores)" << CLR_RESET;
        std::cout << "\033[" << (config_box_y + 5) << ";" << (start_x + 2) << "H"
                  << CLR_SLATE_FG << "• GPU Layers:       " << CLR_TEAL << config_.n_gpu_layers << " (Safe VRAM Headroom Guard)" << CLR_RESET;
        std::cout << "\033[" << (config_box_y + 6) << ";" << (start_x + 2) << "H"
                  << CLR_MUTED << "  All MoE cache, 2Q, TurboQuant, and DFlash2 drafter flags auto-tuned under the hood." << CLR_RESET;
    } else {
        std::cout << "\033[" << (config_box_y + 3) << ";" << (start_x + 2) << "H"
                  << CLR_SLATE_FG << "[-ngl " << config_.n_gpu_layers << "] [-t " << config_.n_threads << "] [-c " << config_.n_ctx << "] [-ctk " << config_.ctk << "] [-ctv " << config_.ctv << "]";
        std::cout << "\033[" << (config_box_y + 4) << ";" << (start_x + 2) << "H"
                  << CLR_SLATE_FG << "Drafter: " << CLR_AMBER << (config_.drafter_path.empty() ? "None" : fs::path(config_.drafter_path).filename().string())
                  << CLR_SLATE_FG << " [-ngld " << config_.drafter_gpu_layers << "]" << CLR_RESET;
        std::cout << "\033[" << (config_box_y + 5) << ";" << (start_x + 2) << "H"
                  << CLR_SLATE_FG << "MoE Slots: " << config_.moe_cache_slots << " | Warm RAM: " << config_.moe_warm_ram_mb << " MB | io_uring: " << (config_.io_uring ? "ON" : "OFF")
                  << " | 2Q: " << (config_.moe_2q ? "ON" : "OFF") << CLR_RESET;
        std::cout << "\033[" << (config_box_y + 6) << ";" << (start_x + 2) << "H"
                  << CLR_MUTED << "  Full custom flag vector will be dispatched directly to the inference engine." << CLR_RESET;
    }

    // Action button
    std::cout << "\033[" << (config_box_y + 8) << ";" << (start_x + 4) << "H"
              << CLR_CARD_BG << CLR_BOLD << CLR_EMERALD << " [ ▶ START INFERENCE (Press Enter) ] " << CLR_RESET;

    // Status / Help Footer
    std::cout << "\033[" << (rows - 1) << ";2H" << CLR_MUTED
              << "Controls: "
              << CLR_BOLD << "[Enter]" << CLR_RESET << CLR_MUTED << " Start  "
              << CLR_BOLD << "[A]" << CLR_RESET << CLR_MUTED << " Auto Mode  "
              << CLR_BOLD << "[M]" << CLR_RESET << CLR_MUTED << " Manual Mode  "
              << CLR_BOLD << "[↑/↓]" << CLR_RESET << CLR_MUTED << " Select Model  "
              << CLR_BOLD << "[Q]" << CLR_RESET << CLR_MUTED << " Quit" << CLR_RESET;
    std::cout << std::flush;
}

void StrataTUI::render_inference_screen() {
    int rows = 0, cols = 0;
    get_terminal_size(rows, cols);

    std::cout << "\033[2J\033[H"; // Clear screen

    // Top Header
    std::cout << "\033[1;1H" << CLR_CARD_BG << CLR_BOLD << CLR_TEAL
              << " 🥪 SANDWICH ENGINE ── LIVE INFERENCE DASHBOARD "
              << CLR_MUTED << "│ "
              << CLR_EMERALD << "● RUNNING "
              << CLR_SLATE_FG << "(" << (config_.auto_mode ? "Auto-Dispatch Mode" : "Manual Mode") << ")"
              << CLR_RESET;

    int box_w = std::min(cols - 4, 100);
    int start_x = 2;

    // Telemetry Dashboard Panel
    int telem_box_h = 7;
    draw_box(3, start_x, telem_box_h, box_w, "REAL-TIME TELEMETRY HUD (Technique #18)", CLR_EMERALD);

    // VRAM Bar
    double vram_pct = (telemetry_.vram_total_mib > 0.0) ? (telemetry_.vram_used_mib / telemetry_.vram_total_mib) * 100.0 : 81.1;
    std::cout << "\033[4;" << (start_x + 2) << "H" << CLR_SLATE_FG << "GPU VRAM: ";
    draw_progress_bar(4, start_x + 12, 24, vram_pct, CLR_TEAL);
    std::cout << " " << std::fixed << std::setprecision(1) << telemetry_.vram_used_mib << " / "
              << telemetry_.vram_total_mib << " MiB (" << vram_pct << "%)";

    // RAM Bar
    double ram_pct = (telemetry_.ram_rss_mib / 24192.0) * 100.0;
    std::cout << "\033[5;" << (start_x + 2) << "H" << CLR_SLATE_FG << "Host RAM: ";
    draw_progress_bar(5, start_x + 12, 24, ram_pct, CLR_CYAN);
    std::cout << " " << std::fixed << std::setprecision(1) << telemetry_.ram_rss_mib << " / 24,192 MiB (" << ram_pct << "%)";

    // Throughput and Cache Telemetry
    std::cout << "\033[6;" << (start_x + 2) << "H"
              << CLR_SLATE_FG << "Decode: " << CLR_EMERALD << CLR_BOLD << telemetry_.decode_tokens_per_sec << " tok/s " << CLR_RESET
              << CLR_SLATE_FG << "│ Prefill: " << CLR_TEAL << telemetry_.prefill_tokens_per_sec << " tok/s "
              << CLR_SLATE_FG << "│ Latency: " << CLR_CYAN << telemetry_.latency_ms << " ms "
              << CLR_SLATE_FG << "│ MoE Hit Rate: " << CLR_AMBER << telemetry_.moe_cache_hit_rate_pct << "%" << CLR_RESET;

    std::cout << "\033[7;" << (start_x + 2) << "H"
              << CLR_MUTED << "Drafter: DFlash2 (VRAM-Pinned -ngld 99) │ Split-KV: Active │ io_uring: Streaming" << CLR_RESET;

    // Streaming Output Box
    int out_box_y = 3 + telem_box_h + 1;
    int out_box_h = std::max(rows - out_box_y - 2, 8);
    draw_box(out_box_y, start_x, out_box_h, box_w, "STREAMING GENERATION OUTPUT", CLR_TEAL);

    // Display output lines (tail of token buffer)
    std::vector<std::string> lines;
    std::istringstream stream(token_output_buffer_);
    std::string l;
    while (std::getline(stream, l)) {
        lines.push_back(l);
    }

    int max_lines = out_box_h - 2;
    int start_line = std::max(0, static_cast<int>(lines.size()) - max_lines);
    for (int i = 0; i < max_lines && (start_line + i) < static_cast<int>(lines.size()); ++i) {
        std::cout << "\033[" << (out_box_y + 1 + i) << ";" << (start_x + 2) << "H"
                  << CLR_SLATE_FG << lines[start_line + i].substr(0, box_w - 4) << CLR_RESET;
    }

    // Footer Controls
    std::cout << "\033[" << (rows - 1) << ";2H" << CLR_MUTED
              << "Controls: "
              << CLR_BOLD << "[S]" << CLR_RESET << CLR_MUTED << " Stop / Back  "
              << CLR_BOLD << "[C]" << CLR_RESET << CLR_MUTED << " Clear Output  "
              << CLR_BOLD << "[Q]" << CLR_RESET << CLR_MUTED << " Quit" << CLR_RESET;

    std::cout << std::flush;
}

void StrataTUI::handle_setup_input(int key) {
    if (key == 'q' || key == 'Q') {
        running_.store(false);
    } else if (key == 'a' || key == 'A') {
        config_.auto_mode = true;
    } else if (key == 'm' || key == 'M') {
        config_.auto_mode = false;
    } else if (key == '\n' || key == '\r') {
        // Start Inference
        current_screen_ = TuiScreen::INFERENCE;
        token_output_buffer_.clear();
        stop_inference_flag_.store(false);
        inference_running_.store(true);

        // Launch inference in background worker thread
        std::thread([this]() {
            strata_start_inference(config_, [this](const std::string & token) {
                token_output_buffer_ += token;
            }, stop_inference_flag_);
            inference_running_.store(false);
        }).detach();
    } else if (key == '\033') { // Escape sequence
        char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) == 1 && read(STDIN_FILENO, &seq[1], 1) == 1) {
            if (seq[0] == '[') {
                if (seq[1] == 'A') { // Up arrow
                    if (selected_model_idx_ > 0) {
                        selected_model_idx_--;
                        config_.model_path = available_models_[selected_model_idx_];
                    }
                } else if (seq[1] == 'B') { // Down arrow
                    if (selected_model_idx_ + 1 < available_models_.size()) {
                        selected_model_idx_++;
                        config_.model_path = available_models_[selected_model_idx_];
                    }
                }
            }
        }
    }
}

void StrataTUI::handle_inference_input(int key) {
    if (key == 'q' || key == 'Q') {
        stop_inference_flag_.store(true);
        running_.store(false);
    } else if (key == 's' || key == 'S') {
        stop_inference_flag_.store(true);
        current_screen_ = TuiScreen::SETUP;
    } else if (key == 'c' || key == 'C') {
        token_output_buffer_.clear();
    }
}

void StrataTUI::run() {
    while (running_.load()) {
        // Poll live telemetry
        telemetry_ = strata_get_telemetry();

        // Render active screen
        if (current_screen_ == TuiScreen::SETUP) {
            render_setup_screen();
        } else {
            render_inference_screen();
        }

        // Non-blocking input read
        char ch = 0;
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n > 0) {
            if (current_screen_ == TuiScreen::SETUP) {
                handle_setup_input(ch);
            } else {
                handle_inference_input(ch);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

} // namespace tui
} // namespace sandwich
