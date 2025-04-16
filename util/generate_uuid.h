#ifndef GENERATE_UUID_H
#define GENERATE_UUID_H

#include <string>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iomanip>

inline std::string generateUUID() {
    // 确保随机数种子只初始化一次
    static bool seeded = false;
    if (!seeded) {
        srand(static_cast<unsigned int>(time(NULL)));
        seeded = true;
    }
    
    // 为UUID的各部分生成随机值
    unsigned int time_low = (rand() << 16) | rand();
    unsigned short time_mid = rand() & 0xFFFF;
    unsigned short time_hi_and_version = (rand() & 0x0FFF) | 0x4000; // 设置版本 (4xxx)
    unsigned short clock_seq = (rand() & 0x3FFF) | 0x8000; // 设置变体位
    unsigned char node[6];
    for (int i = 0; i < 6; ++i) {
        node[i] = rand() & 0xFF;
    }
    
    // 格式化UUID字符串 (8-4-4-4-12)
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    ss << std::setw(8) << time_low << "-";
    ss << std::setw(4) << time_mid << "-";
    ss << std::setw(4) << time_hi_and_version << "-";
    ss << std::setw(4) << clock_seq << "-";
    for (int i = 0; i < 6; ++i) {
        ss << std::setw(2) << static_cast<int>(node[i]);
    }
    
    return ss.str();
}

#endif // GENERATE_UUID_H