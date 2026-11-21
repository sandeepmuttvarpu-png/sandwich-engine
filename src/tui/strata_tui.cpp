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

// Strata Dark-Slate / Teal / Emerald Design Tokens
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
        selected_model_idx_ = 0;
    }
    if (!available_drafters_.empty()) {
        config_.drafter_path = available_drafters_[0];
        selected_drafter_idx_ = 0;
    }
    telemetry_.is_moe = strata_is_model_moe(config_.model_path);
}

StrataTUI::~StrataTUI() {
    shutdown();
}

bool StrataTUI::init() {
    setup_terminal();

    // Probe server health on startup
    server_healthy_ = strata_server_is_healthy("127.0.0.1", 8080);
    if (server_healthy_) {
        server_status_text_ = "Attached to running server (127.0.0.1:8080)";
        strata_query_server_telemetry("127.0.0.1", 8080, telemetry_);
    } else {
        server_status_text_ = "Server offline on :8080 (Press Enter to spawn child & connect)";
        telemetry_ = strata_get_telemetry();
    }
    telemetry_.is_moe = strata_is_model_moe(config_.model_path);
    last_telemetry_poll_ = std::chrono::steady_clock::now();

    return true;
}

void StrataTUI::shutdown() {
    abort_active_generation();
    if (spawned_server_pid_ > 0) {
        strata_server_stop(spawned_server_pid_, 5);
        spawned_server_pid_ = -1;
    }
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

void StrataTUI::copy_cli_command() {
    std::string cmd = strata_construct_cli_command(config_);
    std::string osc52 = strata_make_osc52_copy_sequence(cmd);
    std::cout << osc52 << std::flush;
    toast_message_ = "✓ CLI Command copied to clipboard (OSC52)";
    toast_expire_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(2000);
}

void StrataTUI::abort_active_generation() {
    if (inference_streaming_.load()) {
        abort_inference_flag_.store(true);
        strata_server_abort_slot("127.0.0.1", 8080, 0);
    }
}

void StrataTUI::trigger_model_switch(size_t new_idx) {
    if (new_idx >= available_models_.size()) return;
    selected_model_idx_ = new_idx;
    config_.model_path  = available_models_[new_idx];

    // Reset telemetry accumulators immediately
    strata_reset_telemetry();
    telemetry_ = StrataTelemetry();
    telemetry_.is_moe = strata_is_model_moe(config_.model_path);
    chat_history_.clear();
    active_streaming_text_.clear();

    if (spawned_server_pid_ > 0) {
        // Stop current child server and restart with new model
        strata_server_stop(spawned_server_pid_, 5);
        spawned_server_pid_ = -1;

        current_screen_ = TuiScreen::CONNECTING;
        connect_start_time_ = std::chrono::steady_clock::now();
        spawned_server_pid_ = strata_server_spawn(config_, 8080);
    } else if (server_healthy_) {
        toast_message_ = "Notice: Running external server. Restart server to swap model.";
        toast_expire_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(3000);
    }
}

void StrataTUI::send_current_message() {
    if (input_buffer_.empty() || inference_streaming_.load()) return;

    std::string user_text = input_buffer_;
    input_buffer_.clear();
    input_cursor_ = 0;
    auto_scroll_ = true;
    scroll_offset_ = 0;

    StrataChatMessage user_msg;
    user_msg.role = "user";
    user_msg.content = user_text;
    chat_history_.push_back(user_msg);

    active_streaming_text_.clear();
    abort_inference_flag_.store(false);
    inference_streaming_.store(true);

    // Launch SSE streaming chat thread
    std::thread([this, user_text]() {
        double tok_s = 0.0, lat_ms = 0.0;
        int tok_count = 0;

        bool success = strata_stream_chat_sse("127.0.0.1", 8080, user_text,
            [this](const std::string & chunk) {
                active_streaming_text_ += chunk;
            },
            abort_inference_flag_,
            tok_s, tok_count, lat_ms
        );

        StrataChatMessage asst_msg;
        asst_msg.role = "assistant";
        asst_msg.content = active_streaming_text_;
        asst_msg.token_count = tok_count;
        asst_msg.tok_per_sec = tok_s;
        asst_msg.latency_ms = lat_ms;

        chat_history_.push_back(asst_msg);
        active_streaming_text_.clear();
        inference_streaming_.store(false);
    }).detach();
}

void StrataTUI::render_setup_screen() {
    int rows = 0, cols = 0;
    get_terminal_size(rows, cols);

    std::cout << "\033[2J\033[H"; // Clear screen

    if (cols < 70 || rows < 20) {
        std::cout << "\033[2;2H" << CLR_AMBER << CLR_BOLD
                  << "⚠  TERMINAL WINDOW TOO SMALL (" << cols << "x" << rows << ")" << CLR_RESET;
        std::cout << "\033[4;2H" << CLR_SLATE_FG
                  << "Please resize your terminal window to at least " << CLR_TEAL << "80x24" << CLR_SLATE_FG
                  << " to render the Strata TUI interface." << CLR_RESET;
        std::cout << "\033[6;2H" << CLR_MUTED << "Press [Q] to quit." << CLR_RESET;
        std::cout << std::flush;
        return;
    }

    // Top Header Banner
    std::cout << "\033[1;1H" << CLR_CARD_BG << CLR_BOLD << CLR_TEAL
              << " 🥪 SANDWICH ENGINE ── STRATA TUI "
              << CLR_MUTED << "│ "
              << CLR_SLATE_FG << "Host: HP Victus 15 | i7-13620H (6P+4E) | 24GB DDR5 | RTX 5050 Mobile 8GB"
              << CLR_RESET;

    // Server Status Pill
    std::cout << "\033[2;2H";
    if (server_healthy_) {
        std::cout << CLR_EMERALD << CLR_BOLD << "● " << server_status_text_ << CLR_RESET;
    } else {
        std::cout << CLR_AMBER << CLR_BOLD << "○ " << server_status_text_ << CLR_RESET;
    }

    int box_w = std::min(cols - 4, 100);
    int start_x = 2;

    // Model Selection Panel
    int model_box_h = 7;
    draw_box(4, start_x, model_box_h, box_w, "SELECT TARGET MODEL", CLR_TEAL);

    for (size_t i = 0; i < std::min(available_models_.size(), size_t(4)); ++i) {
        bool selected = (i == selected_model_idx_);
        std::cout << "\033[" << (5 + i) << ";" << (start_x + 2) << "H";
        if (selected) {
            std::cout << CLR_AMBER << CLR_BOLD << "▶ " << CLR_HL_BG << CLR_SLATE_FG;
        } else {
            std::cout << "  " << CLR_MUTED;
        }

        std::string filename = fs::path(available_models_[i]).filename().string();
        bool is_moe = strata_is_model_moe(available_models_[i]);
        std::string tag = is_moe ? "[MoE]" : "[Dense]";

        std::error_code ec;
        auto sz = fs::file_size(available_models_[i], ec);
        double sz_gb = (!ec) ? (static_cast<double>(sz) / (1024.0 * 1024.0 * 1024.0)) : 0.0;

        std::ostringstream item;
        item << filename << " (" << std::fixed << std::setprecision(1) << sz_gb << " GB) " << tag;
        std::string item_str = item.str();
        if (item_str.length() > size_t(box_w - 8)) {
            item_str = item_str.substr(0, box_w - 11) + "...";
        }
        std::cout << item_str << CLR_RESET;
    }

    // Mode Configuration Panel
    int config_box_y = 4 + model_box_h + 1;
    int config_box_h = 8;
    draw_box(config_box_y, start_x, config_box_h, box_w, "DISPATCH CONFIGURATION", CLR_CYAN);

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

    if (config_.auto_mode) {
        std::cout << "\033[" << (config_box_y + 3) << ";" << (start_x + 2) << "H"
                  << CLR_SLATE_FG << "• Context Window:   " << CLR_TEAL << config_.n_ctx << " tokens" << CLR_RESET
                  << CLR_MUTED << "  (Pin 6 P-Cores | 14 GPU layers VRAM Guard)" << CLR_RESET;
        std::cout << "\033[" << (config_box_y + 4) << ";" << (start_x + 2) << "H"
                  << CLR_MUTED << "• MoE 3-tier cache, 2Q, TurboQuant & DFlash2 drafter auto-tuned." << CLR_RESET;
    } else {
        std::cout << "\033[" << (config_box_y + 3) << ";" << (start_x + 2) << "H"
                  << CLR_SLATE_FG << "[-ngl " << config_.n_gpu_layers << "] [-t " << config_.n_threads << "] [-c " << config_.n_ctx << "] [-ctk " << config_.ctk << "] [-ctv " << config_.ctv << "]" << CLR_RESET;
        std::cout << "\033[" << (config_box_y + 4) << ";" << (start_x + 2) << "H"
                  << CLR_SLATE_FG << "MoE Slots: " << config_.moe_cache_slots << " | Warm RAM: " << config_.moe_warm_ram_mb << " MB | io_uring: " << (config_.io_uring ? "ON" : "OFF")
                  << " | 2Q: " << (config_.moe_2q ? "ON" : "OFF") << CLR_RESET;
    }

    std::cout << "\033[" << (config_box_y + 6) << ";" << (start_x + 4) << "H"
              << CLR_CARD_BG << CLR_BOLD << CLR_EMERALD << " [ ▶ CONNECT & LAUNCH CHAT (Press Enter) ] " << CLR_RESET;

    // CLI Command Preview Box
    int cli_box_y = config_box_y + config_box_h + 1;
    int cli_box_h = 4;
    draw_box(cli_box_y, start_x, cli_box_h, box_w, "SYNTHESIZED CLI COMMAND", CLR_BORDER);
    std::string cli_cmd = strata_construct_cli_command(config_);
    if (cli_cmd.length() > size_t(box_w - 6)) {
        cli_cmd = cli_cmd.substr(0, box_w - 9) + "...";
    }
    std::cout << "\033[" << (cli_box_y + 1) << ";" << (start_x + 2) << "H"
              << CLR_MUTED << "$ " << CLR_SLATE_FG << cli_cmd << CLR_RESET;
    std::cout << "\033[" << (cli_box_y + 2) << ";" << (start_x + 2) << "H"
              << CLR_DIM << "Press [C] to copy command to system clipboard via OSC52" << CLR_RESET;

    // Toast notification
    if (!toast_message_.empty() && std::chrono::steady_clock::now() < toast_expire_) {
        std::cout << "\033[" << (rows - 2) << ";2H" << CLR_EMERALD << CLR_BOLD
                  << "  " << toast_message_ << "  " << CLR_RESET;
    }

    // Status / Help Footer
    std::cout << "\033[" << (rows - 1) << ";2H" << CLR_MUTED
              << "Controls: "
              << CLR_BOLD << "[Enter]" << CLR_RESET << CLR_MUTED << " Launch Chat  "
              << CLR_BOLD << "[A]" << CLR_RESET << CLR_MUTED << " Auto  "
              << CLR_BOLD << "[M]" << CLR_RESET << CLR_MUTED << " Manual  "
              << CLR_BOLD << "[↑/↓]" << CLR_RESET << CLR_MUTED << " Select Model  "
              << CLR_BOLD << "[C]" << CLR_RESET << CLR_MUTED << " Copy CLI  "
              << CLR_BOLD << "[Q]" << CLR_RESET << CLR_MUTED << " Quit" << CLR_RESET;
    std::cout << std::flush;
}

void StrataTUI::render_connecting_screen() {
    int rows = 0, cols = 0;
    get_terminal_size(rows, cols);

    std::cout << "\033[2J\033[H";

    static const char * spinners[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
    connecting_spinner_idx_ = (connecting_spinner_idx_ + 1) % 10;

    int box_w = std::min(cols - 4, 72);
    int box_h = 10;
    int start_y = std::max(2, (rows - box_h) / 2);
    int start_x = std::max(2, (cols - box_w) / 2);

    draw_box(start_y, start_x, box_h, box_w, "STARTING INFERENCE SERVER", CLR_TEAL);

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - connect_start_time_).count();

    std::cout << "\033[" << (start_y + 2) << ";" << (start_x + 4) << "H"
              << CLR_CYAN << CLR_BOLD << spinners[connecting_spinner_idx_] << " "
              << CLR_SLATE_FG << "Launching Sandwich Engine on port 8080..." << CLR_RESET;

    std::string mod_name = fs::path(config_.model_path).filename().string();
    std::cout << "\033[" << (start_y + 4) << ";" << (start_x + 4) << "H"
              << CLR_MUTED << "Model: " << CLR_SLATE_FG << mod_name << CLR_RESET;

    std::cout << "\033[" << (start_y + 5) << ";" << (start_x + 4) << "H"
              << CLR_MUTED << "Waiting for /health endpoint (" << elapsed << "s / 60s)..." << CLR_RESET;

    std::cout << "\033[" << (start_y + 7) << ";" << (start_x + 4) << "H"
              << CLR_MUTED << "Press [Esc] to cancel startup" << CLR_RESET;

    std::cout << std::flush;
}

void StrataTUI::render_chat_screen() {
    int rows = 0, cols = 0;
    get_terminal_size(rows, cols);

    std::cout << "\033[2J\033[H";

    if (cols < 70 || rows < 20) {
        std::cout << "\033[2;2H" << CLR_AMBER << CLR_BOLD
                  << "⚠  TERMINAL WINDOW TOO SMALL (" << cols << "x" << rows << ")" << CLR_RESET;
        std::cout << "\033[4;2H" << CLR_SLATE_FG
                  << "Please resize terminal to at least 80x24." << CLR_RESET;
        std::cout << std::flush;
        return;
    }

    // Top Header Banner
    std::string mod_short = fs::path(config_.model_path).filename().string();
    if (mod_short.length() > 24) mod_short = mod_short.substr(0, 21) + "...";
    std::string arch_tag = telemetry_.is_moe ? "[MoE]" : "[Dense]";

    std::cout << "\033[1;1H" << CLR_CARD_BG << CLR_BOLD << CLR_TEAL
              << " 🥪 STRATA CHAT "
              << CLR_MUTED << "│ "
              << CLR_EMERALD << "● 127.0.0.1:8080 "
              << CLR_MUTED << "│ "
              << CLR_SLATE_FG << mod_short << " " << CLR_CYAN << arch_tag << " "
              << CLR_MUTED << "│ [F2/Tab] Toggle HUD"
              << CLR_RESET;

    int hud_w = (hud_visible_ && cols >= 96) ? 34 : 0;
    int chat_w = cols - hud_w - 3;
    int start_x = 2;

    int input_h = 3;
    int chat_h = rows - 2 - input_h;

    // Chat History Box
    draw_box(2, start_x, chat_h, chat_w, "CONVERSATION STREAM", CLR_BORDER);

    // Format all messages to display lines
    std::vector<std::string> display_lines;
    for (const auto & msg : chat_history_) {
        if (msg.role == "user") {
            display_lines.push_back(std::string(CLR_CYAN) + CLR_BOLD + "👤 You:" + CLR_RESET + " " + CLR_SLATE_FG + msg.content + CLR_RESET);
            display_lines.push_back("");
        } else {
            display_lines.push_back(std::string(CLR_EMERALD) + CLR_BOLD + "🤖 Assistant:" + CLR_RESET);
            std::string formatted = strata_format_markdown_lite(msg.content);
            std::istringstream iss(formatted);
            std::string l;
            while (std::getline(iss, l)) {
                display_lines.push_back("  " + l);
            }
            // Message stats footer badge
            std::ostringstream badge;
            badge << CLR_MUTED << "  └─ " << CLR_EMERALD << msg.token_count << " tokens"
                  << CLR_MUTED << " | " << CLR_TEAL << std::fixed << std::setprecision(1) << msg.tok_per_sec << " tok/s"
                  << CLR_MUTED << " | " << CLR_CYAN << msg.latency_ms << " ms" << CLR_RESET;
            display_lines.push_back(badge.str());
            display_lines.push_back("");
        }
    }

    if (inference_streaming_.load()) {
        display_lines.push_back(std::string(CLR_EMERALD) + CLR_BOLD + "🤖 Assistant (streaming...):" + CLR_RESET);
        std::string formatted = strata_format_markdown_lite(active_streaming_text_);
        std::istringstream iss(formatted);
        std::string l;
        while (std::getline(iss, l)) {
            display_lines.push_back("  " + l);
        }
        display_lines.push_back(std::string("  ") + CLR_TEAL + "▌" + CLR_RESET);
    }

    int visible_lines = chat_h - 2;
    int total_lines = static_cast<int>(display_lines.size());
    int max_scroll = std::max(0, total_lines - visible_lines);

    if (auto_scroll_) {
        scroll_offset_ = max_scroll;
    } else {
        scroll_offset_ = std::clamp(scroll_offset_, 0, max_scroll);
    }

    for (int i = 0; i < visible_lines; ++i) {
        int line_idx = scroll_offset_ + i;
        std::cout << "\033[" << (3 + i) << ";" << (start_x + 2) << "H";
        if (line_idx < total_lines) {
            std::string l = display_lines[line_idx];
            // Clip length
            if (l.length() > size_t(chat_w - 4)) {
                l = l.substr(0, chat_w - 7) + "...";
            }
            std::cout << l << CLR_RESET;
        }
    }

    // Input Box
    int input_y = 2 + chat_h;
    draw_box(input_y, start_x, input_h, chat_w, "PROMPT INPUT", CLR_TEAL);
    std::cout << "\033[" << (input_y + 1) << ";" << (start_x + 2) << "H"
              << CLR_TEAL << CLR_BOLD << "❯ " << CLR_RESET << CLR_SLATE_FG << input_buffer_
              << CLR_TEAL << "▌" << CLR_RESET;

    // Right Column: Telemetry HUD Pane
    if (hud_w > 0) {
        int hud_x = start_x + chat_w + 1;
        int hud_h = rows - 2;
        draw_box(2, hud_x, hud_h, hud_w, "TELEMETRY HUD (F2)", CLR_EMERALD);

        // VRAM Bar
        double vram_pct = (telemetry_.vram_total_mib > 0.0) ? (telemetry_.vram_used_mib / telemetry_.vram_total_mib) * 100.0 : 0.0;
        std::cout << "\033[4;" << (hud_x + 2) << "H" << CLR_SLATE_FG << "GPU VRAM:";
        draw_progress_bar(4, hud_x + 12, 16, vram_pct, CLR_TEAL);
        std::cout << "\033[5;" << (hud_x + 2) << "H" << CLR_MUTED << " "
                  << std::fixed << std::setprecision(0) << telemetry_.vram_used_mib << " / "
                  << telemetry_.vram_total_mib << " MiB (" << std::setprecision(1) << vram_pct << "%)" << CLR_RESET;

        // Host RAM Bar
        double ram_pct = (telemetry_.ram_rss_mib / 24192.0) * 100.0;
        std::cout << "\033[7;" << (hud_x + 2) << "H" << CLR_SLATE_FG << "Host RAM:";
        draw_progress_bar(7, hud_x + 12, 16, ram_pct, CLR_CYAN);
        std::cout << "\033[8;" << (hud_x + 2) << "H" << CLR_MUTED << " "
                  << std::fixed << std::setprecision(0) << telemetry_.ram_rss_mib << " / 24,192 MiB ("
                  << std::setprecision(1) << ram_pct << "%)" << CLR_RESET;

        // Token Decode & Latency
        std::cout << "\033[10;" << (hud_x + 2) << "H" << CLR_SLATE_FG << "Decode Speed:" << CLR_RESET;
        std::cout << "\033[10;" << (hud_x + 16) << "H" << CLR_EMERALD << CLR_BOLD
                  << std::fixed << std::setprecision(1) << telemetry_.decode_tokens_per_sec << " tok/s" << CLR_RESET;

        std::cout << "\033[11;" << (hud_x + 2) << "H" << CLR_SLATE_FG << "Token Latency:" << CLR_RESET;
        std::cout << "\033[11;" << (hud_x + 16) << "H" << CLR_CYAN
                  << std::fixed << std::setprecision(1) << telemetry_.latency_ms << " ms" << CLR_RESET;

        // Context Usage
        double ctx_pct = (telemetry_.n_ctx > 0) ? (static_cast<double>(telemetry_.ctx_used_tokens) / telemetry_.n_ctx) * 100.0 : 0.0;
        std::cout << "\033[13;" << (hud_x + 2) << "H" << CLR_SLATE_FG << "Context Used:" << CLR_RESET;
        draw_progress_bar(13, hud_x + 16, 12, ctx_pct, CLR_AMBER);
        std::cout << "\033[14;" << (hud_x + 2) << "H" << CLR_MUTED << " "
                  << telemetry_.ctx_used_tokens << " / " << telemetry_.n_ctx << " ("
                  << std::fixed << std::setprecision(1) << ctx_pct << "%)" << CLR_RESET;

        // MoE Hit Rate
        std::cout << "\033[16;" << (hud_x + 2) << "H" << CLR_SLATE_FG << "MoE Hit Rate:" << CLR_RESET;
        std::cout << "\033[16;" << (hud_x + 16) << "H";
        if (!telemetry_.is_moe) {
            std::cout << CLR_MUTED << "N/A (dense)" << CLR_RESET;
        } else {
            std::cout << CLR_AMBER << CLR_BOLD << std::fixed << std::setprecision(1)
                      << telemetry_.moe_cache_hit_rate_pct << "%" << CLR_RESET;
        }

        // Feature flags badge
        std::cout << "\033[18;" << (hud_x + 2) << "H" << CLR_BORDER << "─────────────────────────────" << CLR_RESET;
        std::cout << "\033[19;" << (hud_x + 2) << "H" << CLR_MUTED << "Drafter : " << CLR_TEAL << "DFlash2 Pinned" << CLR_RESET;
        std::cout << "\033[20;" << (hud_x + 2) << "H" << CLR_MUTED << "Offload : " << CLR_TEAL << "Asym Split-KV" << CLR_RESET;
        std::cout << "\033[21;" << (hud_x + 2) << "H" << CLR_MUTED << "Storage : " << CLR_TEAL << "io_uring + 2Q" << CLR_RESET;
    }

    // Toast message if active
    if (!toast_message_.empty() && std::chrono::steady_clock::now() < toast_expire_) {
        std::cout << "\033[" << (rows - 2) << ";4H" << CLR_EMERALD << CLR_BOLD
                  << "  " << toast_message_ << "  " << CLR_RESET;
    }

    // Bottom Help Controls
    std::cout << "\033[" << (rows - 1) << ";2H" << CLR_MUTED
              << "Controls: "
              << CLR_BOLD << "[Enter]" << CLR_RESET << CLR_MUTED << " Send  "
              << CLR_BOLD << "[Esc]" << CLR_RESET << CLR_MUTED << " " << (inference_streaming_.load() ? "Abort Gen" : "Back to Setup") << "  "
              << CLR_BOLD << "[F2]" << CLR_RESET << CLR_MUTED << " Toggle HUD  "
              << CLR_BOLD << "[PgUp/Dn]" << CLR_RESET << CLR_MUTED << " Scroll  "
              << CLR_BOLD << "[Q]" << CLR_RESET << CLR_MUTED << " Quit" << CLR_RESET;
    std::cout << std::flush;
}

void StrataTUI::render_shutdown_confirm_screen() {
    int rows = 0, cols = 0;
    get_terminal_size(rows, cols);

    std::cout << "\033[2J\033[H";

    int box_w = 48;
    int box_h = 9;
    int start_y = std::max(2, (rows - box_h) / 2);
    int start_x = std::max(2, (cols - box_w) / 2);

    draw_box(start_y, start_x, box_h, box_w, "SHUTDOWN ENGINE SERVER?", CLR_ROSE);

    std::cout << "\033[" << (start_y + 2) << ";" << (start_x + 4) << "H"
              << CLR_SLATE_FG << "Sandwich child server is running (PID: " << spawned_server_pid_ << ")." << CLR_RESET;

    std::cout << "\033[" << (start_y + 4) << ";" << (start_x + 4) << "H"
              << CLR_BOLD << "Stop server process on exit?" << CLR_RESET;

    std::cout << "\033[" << (start_y + 6) << ";" << (start_x + 4) << "H"
              << CLR_ROSE << CLR_BOLD << "[Y] Stop & Quit" << CLR_RESET
              << "    " << CLR_MUTED << "[N] Leave Running & Quit" << CLR_RESET;

    std::cout << "\033[" << (start_y + 7) << ";" << (start_x + 4) << "H"
              << CLR_DIM << "Press [Esc] to cancel exit" << CLR_RESET;

    std::cout << std::flush;
}

void StrataTUI::handle_setup_input(int key) {
    if (key == 'q' || key == 'Q') {
        if (spawned_server_pid_ > 0) {
            current_screen_ = TuiScreen::SHUTDOWN_CONFIRM;
        } else {
            running_.store(false);
        }
    } else if (key == 'a' || key == 'A') {
        config_.auto_mode = true;
    } else if (key == 'm' || key == 'M') {
        config_.auto_mode = false;
    } else if (key == 'c' || key == 'C') {
        copy_cli_command();
    } else if (key == '\n' || key == '\r') {
        // Connect to server or spawn server
        if (strata_server_is_healthy("127.0.0.1", 8080)) {
            server_healthy_ = true;
            current_screen_ = TuiScreen::CHAT;
        } else {
            // Spawn child server
            current_screen_ = TuiScreen::CONNECTING;
            connect_start_time_ = std::chrono::steady_clock::now();
            spawned_server_pid_ = strata_server_spawn(config_, 8080);
        }
    } else if (key == '\033') { // Escape sequence
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) == 1 && read(STDIN_FILENO, &seq[1], 1) == 1) {
            if (seq[0] == '[') {
                if (seq[1] == 'A') { // Up arrow
                    if (selected_model_idx_ > 0) {
                        trigger_model_switch(selected_model_idx_ - 1);
                    }
                } else if (seq[1] == 'B') { // Down arrow
                    if (selected_model_idx_ + 1 < available_models_.size()) {
                        trigger_model_switch(selected_model_idx_ + 1);
                    }
                }
            }
        }
    }
}

void StrataTUI::handle_connecting_input(int key) {
    if (key == '\033' || key == 'q' || key == 'Q') {
        if (spawned_server_pid_ > 0) {
            strata_server_stop(spawned_server_pid_, 2);
            spawned_server_pid_ = -1;
        }
        current_screen_ = TuiScreen::SETUP;
    }
}

void StrataTUI::handle_chat_input(int key) {
    if (key == '\033') { // Escape or escape sequence
        char seq[4] = {0};
        if (read(STDIN_FILENO, &seq[0], 1) == 1) {
            if (seq[0] == '[') {
                if (read(STDIN_FILENO, &seq[1], 1) == 1) {
                    if (seq[1] == 'A') { // Up arrow
                        auto_scroll_ = false;
                        scroll_offset_ = std::max(0, scroll_offset_ - 2);
                    } else if (seq[1] == 'B') { // Down arrow
                        scroll_offset_ += 2;
                    } else if (seq[1] == '5' && read(STDIN_FILENO, &seq[2], 1) == 1 && seq[2] == '~') { // PgUp
                        auto_scroll_ = false;
                        scroll_offset_ = std::max(0, scroll_offset_ - 10);
                    } else if (seq[1] == '6' && read(STDIN_FILENO, &seq[2], 1) == 1 && seq[2] == '~') { // PgDn
                        scroll_offset_ += 10;
                    } else if (seq[1] == '1' && read(STDIN_FILENO, &seq[2], 1) == 1 && seq[2] == '2' && read(STDIN_FILENO, &seq[3], 1) == 1 && seq[3] == '~') { // F2
                        hud_visible_ = !hud_visible_;
                    }
                }
            } else if (seq[0] == 'O') {
                if (read(STDIN_FILENO, &seq[1], 1) == 1 && seq[1] == 'Q') { // F2 on some terminals
                    hud_visible_ = !hud_visible_;
                }
            }
        } else {
            // Standalone Escape pressed
            if (inference_streaming_.load()) {
                abort_active_generation();
            } else {
                current_screen_ = TuiScreen::SETUP;
            }
        }
    } else if (key == 3) { // Ctrl+C
        if (inference_streaming_.load()) {
            abort_active_generation();
        } else {
            if (spawned_server_pid_ > 0) {
                current_screen_ = TuiScreen::SHUTDOWN_CONFIRM;
            } else {
                running_.store(false);
            }
        }
    } else if (key == '\t') { // Tab toggles HUD
        hud_visible_ = !hud_visible_;
    } else if (key == '\n' || key == '\r') {
        send_current_message();
    } else if (key == 127 || key == '\b') { // Backspace
        if (!input_buffer_.empty()) {
            input_buffer_.pop_back();
        }
    } else if (key == 21) { // Ctrl+U
        input_buffer_.clear();
    } else if (key >= 32 && key <= 126) {
        input_buffer_.push_back(static_cast<char>(key));
    }
}

void StrataTUI::handle_shutdown_confirm_input(int key) {
    if (key == 'y' || key == 'Y') {
        if (spawned_server_pid_ > 0) {
            strata_server_stop(spawned_server_pid_, 5);
            spawned_server_pid_ = -1;
        }
        running_.store(false);
    } else if (key == 'n' || key == 'N') {
        // Leave server running
        spawned_server_pid_ = -1;
        running_.store(false);
    } else if (key == '\033' || key == 'c' || key == 'C') {
        current_screen_ = TuiScreen::SETUP;
    }
}

void StrataTUI::run() {
    while (running_.load()) {
        auto now = std::chrono::steady_clock::now();

        // Screen-specific state updates
        if (current_screen_ == TuiScreen::CONNECTING) {
            if (strata_server_is_healthy("127.0.0.1", 8080)) {
                server_healthy_ = true;
                server_status_text_ = "Attached to running server (127.0.0.1:8080)";
                current_screen_ = TuiScreen::CHAT;
            } else {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - connect_start_time_).count();
                if (elapsed > 60) {
                    current_screen_ = TuiScreen::SETUP;
                    toast_message_ = "Error: Server connection timed out after 60s";
                    toast_expire_ = now + std::chrono::milliseconds(3000);
                }
            }
        } else if (current_screen_ == TuiScreen::CHAT || current_screen_ == TuiScreen::SETUP) {
            // Poll server & telemetry at 4 Hz (every 250ms)
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_telemetry_poll_).count() > 250) {
                last_telemetry_poll_ = now;
                server_healthy_ = strata_server_is_healthy("127.0.0.1", 8080);
                if (server_healthy_) {
                    strata_query_server_telemetry("127.0.0.1", 8080, telemetry_);
                } else {
                    telemetry_ = strata_get_telemetry();
                }
                telemetry_.is_moe = strata_is_model_moe(config_.model_path);
            }
        }

        // Render current screen
        switch (current_screen_) {
            case TuiScreen::SETUP:
                render_setup_screen();
                break;
            case TuiScreen::CONNECTING:
                render_connecting_screen();
                break;
            case TuiScreen::CHAT:
                render_chat_screen();
                break;
            case TuiScreen::SHUTDOWN_CONFIRM:
                render_shutdown_confirm_screen();
                break;
        }

        // Non-blocking input read
        char ch = 0;
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n > 0) {
            switch (current_screen_) {
                case TuiScreen::SETUP:
                    handle_setup_input(ch);
                    break;
                case TuiScreen::CONNECTING:
                    handle_connecting_input(ch);
                    break;
                case TuiScreen::CHAT:
                    handle_chat_input(ch);
                    break;
                case TuiScreen::SHUTDOWN_CONFIRM:
                    handle_shutdown_confirm_input(ch);
                    break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

} // namespace tui
} // namespace sandwich
