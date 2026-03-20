#include "spotify_tui/utils/platform.hpp"
#include <iostream>
#include <fstream>
#include <cstdlib>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <shellapi.h>
    #include <process.h>
    #include <io.h>
    #define popen _popen
    #define pclose _pclose
#else
    #include <sys/stat.h>
    #include <sys/wait.h>
    #include <unistd.h>
    #include <stdlib.h>
    #include <signal.h>
    #include <stdio.h>
#endif

namespace spotify_tui {
namespace platform {

static std::function<void()> g_exit_handler;

#ifdef _WIN32
static HANDLE g_job_handle = nullptr;

static void cleanup_job() {
    if (g_job_handle) {
        CloseHandle(g_job_handle);
        g_job_handle = nullptr;
    }
}
#endif

std::string get_config_dir() {
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        return std::string(appdata) + "\\SpotiTUI";
    }
    return "C:\\Users\\Public\\SpotiTUI";
#else
    const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
    if (xdg_config) {
        return std::string(xdg_config) + "/SpotiTUI";
    }
    const char* home = std::getenv("HOME");
    if (home) {
        return std::string(home) + "/.config/SpotiTUI";
    }
    return ".config/SpotiTUI";
#endif
}

std::string get_temp_dir() {
#ifdef _WIN32
    char temp_path[MAX_PATH];
    if (GetTempPathA(MAX_PATH, temp_path)) {
        return std::string(temp_path);
    }
    return "C:\\Temp";
#else
    const char* tmpdir = std::getenv("TMPDIR");
    if (tmpdir) return tmpdir;
    return "/tmp";
#endif
}

std::string get_log_path() {
    return get_temp_dir() + "/spotiTUI.log";
}

void open_url(const std::string& url) {
#ifdef _WIN32
    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
    std::string cmd = "xdg-open \"" + url + "\"";
    std::system(cmd.c_str());
#endif
}

struct Process::Impl {
#ifdef _WIN32
    HANDLE handle = nullptr;
    HANDLE stdout_read = nullptr;
    HANDLE stdout_write = nullptr;
    bool running = false;
#else
    pid_t pid = -1;
    FILE* output = nullptr;
    bool running = false;
#endif
};

Process::Process() : impl_(std::make_unique<Impl>()) {}

Process::~Process() {
    stop();
}

bool Process::start(const std::string& executable, const std::vector<std::string>& args, bool redirect_output) {
#ifdef _WIN32
    if (redirect_output) {
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = nullptr;

        if (!CreatePipe(&impl_->stdout_read, &impl_->stdout_write, &sa, 0)) {
            return false;
        }
    }

    std::string cmd_line = executable;
    for (const auto& arg : args) {
        cmd_line += " \"" + arg + "\"";
    }

    STARTUPINFOA si = {0};
    si.cb = sizeof(STARTUPINFO);
    if (redirect_output) {
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = impl_->stdout_write;
        si.hStdError = impl_->stdout_write;
    }

    PROCESS_INFORMATION pi = {0};

    if (!CreateProcessA(executable.c_str(), cmd_line.data(), nullptr, nullptr, 
                       redirect_output ? TRUE : FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        if (redirect_output) {
            CloseHandle(impl_->stdout_read);
            CloseHandle(impl_->stdout_write);
        }
        return false;
    }

    // Create a job object to ensure child process dies with us
    if (!g_job_handle) {
        g_job_handle = CreateJobObjectA(nullptr, nullptr);
        if (g_job_handle) {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {0};
            jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            SetInformationJobObject(g_job_handle, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
            atexit(cleanup_job);
        }
    }
    if (g_job_handle) {
        AssignProcessToJobObject(g_job_handle, pi.hProcess);
    }

    impl_->handle = pi.hProcess;
    impl_->running = true;

    if (redirect_output) {
        CloseHandle(impl_->stdout_write);
    }

    CloseHandle(pi.hThread);
    return true;
#else
    impl_->pid = fork();
    
    if (impl_->pid < 0) {
        return false;
    }
    
    if (impl_->pid == 0) {
        if (redirect_output) {
            FILE* log_file = fopen(get_log_path().c_str(), "a");
            if (log_file) {
                dup2(fileno(log_file), STDOUT_FILENO);
                dup2(fileno(log_file), STDERR_FILENO);
                fclose(log_file);
            }
        }
        
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(executable.c_str()));
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        
        execvp(executable.c_str(), argv.data());
        _exit(1);
    }
    
    impl_->running = true;
    return true;
#endif
}

void Process::stop() {
#ifdef _WIN32
    if (impl_->handle) {
        TerminateProcess(impl_->handle, 0);
        WaitForSingleObject(impl_->handle, INFINITE);
        CloseHandle(impl_->handle);
        impl_->handle = nullptr;
    }
    if (impl_->stdout_read) {
        CloseHandle(impl_->stdout_read);
        impl_->stdout_read = nullptr;
    }
    impl_->running = false;
#else
    if (impl_->pid > 0) {
        kill(impl_->pid, SIGTERM);
        waitpid(impl_->pid, nullptr, 0);
        impl_->pid = -1;
    }
    impl_->running = false;
#endif
}

bool Process::is_running() const {
#ifdef _WIN32
    if (!impl_->handle) return false;
    DWORD exit_code;
    if (GetExitCodeProcess(impl_->handle, &exit_code)) {
        return exit_code == STILL_ACTIVE;
    }
    return false;
#else
    if (impl_->pid <= 0) return false;
    int status;
    return waitpid(impl_->pid, &status, WNOHANG) == 0;
#endif
}

int Process::wait() {
#ifdef _WIN32
    if (impl_->handle) {
        WaitForSingleObject(impl_->handle, INFINITE);
        DWORD exit_code = 0;
        GetExitCodeProcess(impl_->handle, &exit_code);
        CloseHandle(impl_->handle);
        impl_->handle = nullptr;
        impl_->running = false;
        return exit_code;
    }
    return -1;
#else
    if (impl_->pid > 0) {
        int status;
        waitpid(impl_->pid, &status, 0);
        impl_->pid = -1;
        impl_->running = false;
        return WEXITSTATUS(status);
    }
    return -1;
#endif
}

#ifdef _WIN32
static std::function<void()> g_exit_handler;

static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_CLOSE_EVENT) {
        if (g_exit_handler) {
            g_exit_handler();
        }
        return TRUE;
    }
    return FALSE;
}
#endif

void setup_signal_handler(std::function<void()> on_exit) {
    g_exit_handler = on_exit;
#ifdef _WIN32
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#else
    signal(SIGINT, [](int) { g_exit_handler(); });
    signal(SIGTERM, [](int) { g_exit_handler(); });
#endif
}

}
}
