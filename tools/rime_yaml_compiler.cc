/*
 * rime_yaml_compiler.cc
 * RIME YAML 编译工具
 * 用于编译自定义的 YAML 配置文件
 */
#include <iostream>
#include <string>
#include <filesystem>
#include <rime/config.h>
#include <rime/config/plugins.h>
#include <rime/schema.h>
#include "codepage.h"

using namespace rime;
using namespace std;

void print_usage(const char* program_name) {
    cout << "用法: " << program_name << " [选项] <YAML文件路径>" << endl;
    cout << "选项:" << endl;
    cout << "  -s, --source <路径>     源路径 (默认: 当前目录)" << endl;
    cout << "  -d, --dest <路径>       目标路径 (默认: 当前目录)" << endl;
    cout << "  -h, --help             显示此帮助信息" << endl;
    cout << endl;
    cout << "示例:" << endl;
    cout << "  " << program_name << " theme/bim.yaml" << endl;
    cout << "  " << program_name << " -s /path/to/source -d /path/to/dest config.yaml" << endl;
}

bool compile_yaml_file(const string& src_path, const string& dest_path, const string& file_path) {
    try {
        // 确保目标目录存在
        filesystem::path dest_dir(dest_path);
        if (!filesystem::exists(dest_dir)) {
            filesystem::create_directories(dest_dir);
            cout << "创建目标目录: " << dest_path << endl;
        }

        // 创建配置构建器
        auto config_builder = new ConfigComponent<ConfigBuilder>([&](ConfigBuilder* builder) {
            builder->InstallPlugin(new AutoPatchConfigPlugin);
            builder->InstallPlugin(new DefaultConfigPlugin);
            builder->InstallPlugin(new LegacyPresetConfigPlugin);
            builder->InstallPlugin(new LegacyDictionaryConfigPlugin);
            builder->InstallPlugin(new BuildInfoPlugin);
            builder->InstallPlugin(new SaveOutputPlugin(dest_path));
        }, src_path);

        // 编译文件
        cout << "正在编译 YAML 文件: " << file_path << endl;
        cout << "源路径: " << src_path << endl;
        cout << "目标路径: " << dest_path << endl;
        
        bool result = config_builder->Create(file_path);
        
        if (result) {
            cout << "✓ 编译成功!" << endl;
        } else {
            cerr << "✗ 编译失败!" << endl;
        }
        
        delete config_builder;
        return result;
        
    } catch (const exception& e) {
        cerr << "错误: " << e.what() << endl;
        return false;
    }
}

int main(int argc, char* argv[]) {
    unsigned int codepage = SetConsoleOutputCodePage();
    
    string src_path = ".";
    string dest_path = ".";
    string file_path;
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            SetConsoleOutputCodePage(codepage);
            return 0;
        } else if (arg == "-s" || arg == "--source") {
            if (i + 1 < argc) {
                src_path = argv[++i];
            } else {
                cerr << "错误: " << arg << " 需要一个参数" << endl;
                SetConsoleOutputCodePage(codepage);
                return 1;
            }
        } else if (arg == "-d" || arg == "--dest") {
            if (i + 1 < argc) {
                dest_path = argv[++i];
            } else {
                cerr << "错误: " << arg << " 需要一个参数" << endl;
                SetConsoleOutputCodePage(codepage);
                return 1;
            }
        } else if (arg[0] != '-') {
            // 这是文件路径
            if (file_path.empty()) {
                file_path = arg;
            } else {
                cerr << "错误: 指定了多个文件路径" << endl;
                SetConsoleOutputCodePage(codepage);
                return 1;
            }
        } else {
            cerr << "错误: 未知选项 " << arg << endl;
            print_usage(argv[0]);
            SetConsoleOutputCodePage(codepage);
            return 1;
        }
    }
    
    // 检查是否指定了文件路径
    if (file_path.empty()) {
        cerr << "错误: 必须指定 YAML 文件路径" << endl;
        print_usage(argv[0]);
        SetConsoleOutputCodePage(codepage);
        return 1;
    }
    
    // 检查文件是否存在
    string full_file_path = src_path + "/" + file_path;
    if (!filesystem::exists(full_file_path)) {
        cerr << "错误: 文件不存在: " << full_file_path << endl;
        SetConsoleOutputCodePage(codepage);
        return 1;
    }
    
    cout << "=== RIME YAML 编译器 ===" << endl;
    
    // 编译文件
    bool success = compile_yaml_file(src_path, dest_path, file_path);
    
    SetConsoleOutputCodePage(codepage);
    return success ? 0 : 1;
} 