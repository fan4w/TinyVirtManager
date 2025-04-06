#include "./log.h"
#include "../conf/config_manager.h"

Log::Log() {
    lineCount_ = 0;
    isOpen_ = false;
    level_ = 0;
    fp_ = nullptr;
}

Log::~Log() {
    if ( fp_ ) {
        flush();
        fclose(fp_);
    }
}

void Log::init(int level, const char* path, const char* suffix) {
    level_ = level;

    if ( path != nullptr ) {
        path_ = path;
        char dirPath[LOG_PATH_LEN] = { 0 };
        strcpy(dirPath, path);
        if ( mkdir(dirPath, 0777) == -1 && errno != EEXIST ) {
            return;
        }
    }

    if ( suffix != nullptr ) {
        suffix_ = suffix;
    }

    MAX_LINES_ = MAX_LINES;

    // 获取当前时间
    time_t timer = time(nullptr);
    struct tm* sysTime = localtime(&timer);
    struct tm t = *sysTime;

    char fileName[LOG_NAME_LEN] = { 0 };
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
        path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);

    toDay_ = t.tm_mday;

    {
        std::lock_guard<std::mutex> locker(mtx_);
        if ( fp_ ) {
            flush();
            fclose(fp_);
        }

        fp_ = fopen(fileName, "a");
        if ( fp_ == nullptr ) {
            mkdir(path_, 0777);
            fp_ = fopen(fileName, "a");
        }

        isOpen_ = true;
    }
}

void Log::initFromConfig(const char* configPath) {
    // 初始化配置管理器
    ConfigManager* configManager = ConfigManager::Instance();
    if ( !configManager->init(configPath) ) {
        // 配置文件加载失败，使用默认配置
        init(1, "./log", ".log");
        return;
    }

    // 从配置文件读取日志配置
    int level = configManager->getIntValue("log.level", 1);
    std::string path = configManager->getValue("log.path", "./log");
    std::string suffix = configManager->getValue("log.extension", ".log");
    int maxLines = configManager->getIntValue("log.max_lines", MAX_LINES);

    // 使用配置的参数初始化日志系统
    init(level, path.c_str(), suffix.c_str());
    MAX_LINES_ = maxLines;

    // 记录一条初始化日志
    LOG_INFO("Log system initialized from config file: %s", configPath);
}

Log* Log::Instance() {
    static Log instance;
    return &instance;
}

void Log::write(int level, const char* format, ...) {
    struct timeval now = { 0, 0 };
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm* sysTime = localtime(&tSec);
    struct tm t = *sysTime;

    // 日期改变或行数超过限制，创建新文件
    if ( toDay_ != t.tm_mday || (lineCount_ > MAX_LINES_ && MAX_LINES_ > 0) ) {
        char newFile[LOG_PATH_LEN] = { 0 };
        char tail[36] = { 0 };
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if ( toDay_ != t.tm_mday ) {
            snprintf(newFile, LOG_PATH_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            toDay_ = t.tm_mday;
            lineCount_ = 0;
        }
        else {
            snprintf(newFile, LOG_PATH_LEN - 72, "%s/%s-%d%s", path_, tail, (lineCount_ / MAX_LINES_), suffix_);
        }

        std::lock_guard<std::mutex> locker(mtx_);
        flush();
        fclose(fp_);
        fp_ = fopen(newFile, "a");
        assert(fp_ != nullptr);
    }

    {
        std::lock_guard<std::mutex> locker(mtx_);
        lineCount_++;

        // 写入日志时间内容
        int n = snprintf(buff_.BeginWrite(), 128, "%04d-%02d-%02d %02d:%02d:%02d.%06ld ",
            t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
            t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        buff_.HasWritten(n);
        AppendLogLevelTitle_(level);

        // 可变参数列表
        va_list vaList;
        va_start(vaList, format);
        int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, vaList);
        va_end(vaList);
        buff_.HasWritten(m);
        buff_.Append("\n\0", 2);

        fputs(buff_.Peek(), fp_);
        buff_.RetrieveAll();
    }
}

void Log::AppendLogLevelTitle_(int level) {
    switch ( level ) {
    case 0:
        buff_.Append("[debug]: ", 9);
        break;
    case 1:
        buff_.Append("[info] : ", 9);
        break;
    case 2:
        buff_.Append("[warn] : ", 9);
        break;
    case 3:
        buff_.Append("[error]: ", 9);
        break;
    default:
        buff_.Append("[info] : ", 9);
        break;
    }
}

void Log::flush() {
    if ( fp_ ) {
        // 将缓冲区的数据立即写入文件或设备中
        fflush(fp_);
    }
}

int Log::GetLevel() {
    return level_;
}

void Log::SetLevel(int level) {
    level_ = level;
}