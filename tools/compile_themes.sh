#!/bin/bash

# RIME 主题批量编译脚本
# 用于编译 space/themes 目录下的所有主题文件

# 设置颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 脚本配置
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/cmake-build-debug"
COMPILER="$BUILD_DIR/bin/rime_yaml_compiler"

# 默认路径配置
DEFAULT_SOURCE_DIR="$BUILD_DIR/bin/space/themes"
DEFAULT_BUILD_DIR="$BUILD_DIR/bin/space/themes/build"
THEME_FILE="theme.yaml"

# 函数：显示使用说明
show_usage() {
    echo -e "${BLUE}RIME 主题批量编译工具${NC}"
    echo ""
    echo "用法: $0 [选项] [主题名...]"
    echo ""
    echo "选项:"
    echo "  -s, --source <目录>    源目录路径 (默认: space/themes)"
    echo "  -d, --dest <目录>      编译输出目录 (默认: space/themes/build)"
    echo "  -c, --compiler <路径>  编译器路径 (默认: 自动检测)"
    echo "  -l, --list            列出所有可用主题"
    echo "  -h, --help            显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  $0                     # 编译所有主题"
    echo "  $0 microsoft default   # 只编译指定主题"
    echo "  $0 -l                  # 列出所有主题"
    echo "  $0 -s custom/themes microsoft  # 使用自定义源目录"
    echo ""
}

# 函数：检查编译器是否存在
check_compiler() {
    if [[ ! -f "$COMPILER" ]]; then
        echo -e "${RED}错误: 找不到编译器 $COMPILER${NC}"
        echo -e "${YELLOW}请先编译项目: cd cmake-build-debug && ninja rime_yaml_compiler${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓ 编译器已找到: $COMPILER${NC}"
}

# 函数：列出所有可用主题
list_themes() {
    echo -e "${BLUE}可用主题列表:${NC}"
    if [[ -d "$SOURCE_DIR" ]]; then
        local count=0
        for theme_dir in "$SOURCE_DIR"/*/; do
            if [[ -d "$theme_dir" ]]; then
                local theme_name=$(basename "$theme_dir")
                # 跳过 build 目录和隐藏目录
                if [[ "$theme_name" != "build" && "$theme_name" != .* ]]; then
                    if [[ -f "$theme_dir/$THEME_FILE" ]]; then
                        echo -e "  ${GREEN}✓${NC} $theme_name"
                        ((count++))
                    else
                        echo -e "  ${YELLOW}?${NC} $theme_name (缺少 $THEME_FILE)"
                    fi
                fi
            fi
        done
        echo -e "\n总计: $count 个主题"
    else
        echo -e "${RED}错误: 源目录不存在: $SOURCE_DIR${NC}"
        exit 1
    fi
}

# 函数：编译单个主题
compile_theme() {
    local theme_name="$1"
    local theme_dir="$SOURCE_DIR/$theme_name"
    local theme_file="$theme_dir/$THEME_FILE"
    local info_file="$theme_dir/info.yaml"
    local output_dir="$BUILD_DIR/$theme_name"
    
    echo -e "${BLUE}[编译] $theme_name${NC}"
    
    # 检查主题目录和文件是否存在
    if [[ ! -d "$theme_dir" ]]; then
        echo -e "${RED}  ✗ 主题目录不存在: $theme_dir${NC}"
        return 1
    fi
    
    if [[ ! -f "$theme_file" ]]; then
        echo -e "${RED}  ✗ 主题文件不存在: $theme_file${NC}"
        return 1
    fi
    
    # 确保输出目录存在
    mkdir -p "$output_dir"
    
    local compilation_success=0
    local tasks_count=0
    local success_count=0
    
    # 编译 theme.yaml
    echo -e "${BLUE}    [1/3] 编译 theme.yaml${NC}"
    local relative_theme_path="$theme_name/$THEME_FILE"
    if "$COMPILER" -s "$SOURCE_DIR" -d "$BUILD_DIR" "$relative_theme_path" >/dev/null 2>&1; then
        echo -e "${GREEN}    ✓ theme.yaml 编译成功${NC}"
        ((success_count++))
    else
        echo -e "${RED}    ✗ theme.yaml 编译失败${NC}"
        return 1
    fi
    ((tasks_count++))
    
    # 编译 info.yaml（如果存在）
    echo -e "${BLUE}    [2/3] 编译 info.yaml${NC}"
    if [[ -f "$info_file" ]]; then
        local relative_info_path="$theme_name/info.yaml"
        local compile_output
        local compile_exit_code
        
        # 执行编译并捕获输出和退出码
        compile_output=$("$COMPILER" -s "$SOURCE_DIR" -d "$BUILD_DIR" "$relative_info_path" 2>&1)
        compile_exit_code=$?
        
        # 检查编译是否真正成功：输出文件必须存在且没有错误信息
        if [[ -f "$output_dir/info.yaml" ]] && [[ ! "$compile_output" =~ "error building config" ]]; then
            echo -e "${GREEN}    ✓ info.yaml 编译成功${NC}"
            ((success_count++))
        else
            echo -e "${YELLOW}    ⚠ info.yaml 编译失败，复制原始文件${NC}"
            # 编译失败时复制原始文件
            cp "$info_file" "$output_dir/info.yaml" 2>/dev/null
            if [[ -f "$output_dir/info.yaml" ]]; then
                ((success_count++))
            fi
        fi
    else
        echo -e "${YELLOW}    - info.yaml 不存在，跳过${NC}"
    fi
    ((tasks_count++))
    
    # 复制所有非 yaml 文件到输出目录
    echo -e "${BLUE}    [3/3] 复制其他文件${NC}"
    local copied_files=0
    local copy_errors=0
    
    while IFS= read -r -d '' file; do
        # 计算相对路径
        local relative_path="${file#$theme_dir/}"
        local dest_file="$output_dir/$relative_path"
        local dest_dir="$(dirname "$dest_file")"
        
        # 创建目标目录
        mkdir -p "$dest_dir"
        
        # 复制文件
        if cp "$file" "$dest_file" 2>/dev/null; then
            ((copied_files++))
        else
            ((copy_errors++))
        fi
    done < <(find "$theme_dir" -type f ! -name "*.yaml" -print0 2>/dev/null)
    
    if [[ $copied_files -gt 0 ]]; then
        echo -e "${GREEN}    ✓ 复制了 $copied_files 个附加文件${NC}"
        ((success_count++))
    elif [[ $copy_errors -gt 0 ]]; then
        echo -e "${YELLOW}    ⚠ 复制文件时有 $copy_errors 个错误${NC}"
    else
        echo -e "${YELLOW}    - 没有找到需要复制的文件${NC}"
    fi
    ((tasks_count++))
    
    echo -e "${GREEN}  ✓ 主题处理完成 ($success_count/$tasks_count 成功)${NC}"
    return 0
}

# 函数：批量编译主题
compile_themes() {
    local themes=("$@")
    local success_count=0
    local fail_count=0
    local total_count=0
    
    # 如果没有指定主题，则编译所有主题
    if [[ ${#themes[@]} -eq 0 ]]; then
        echo -e "${BLUE}编译所有主题...${NC}"
        for theme_dir in "$SOURCE_DIR"/*/; do
            if [[ -d "$theme_dir" ]]; then
                local theme_name=$(basename "$theme_dir")
                # 跳过 build 目录和隐藏目录
                if [[ "$theme_name" != "build" && "$theme_name" != .* ]]; then
                    if [[ -f "$theme_dir/$THEME_FILE" ]]; then
                        themes+=("$theme_name")
                    fi
                fi
            fi
        done
    fi
    
    total_count=${#themes[@]}
    echo -e "${BLUE}准备编译 $total_count 个主题${NC}"
    echo ""
    
    # 编译每个主题
    for theme in "${themes[@]}"; do
        if compile_theme "$theme"; then
            ((success_count++))
        else
            ((fail_count++))
        fi
        echo ""
    done
    
    # 显示编译结果统计
    echo -e "${BLUE}=== 编译完成 ===${NC}"
    echo -e "总计: $total_count"
    echo -e "${GREEN}成功: $success_count${NC}"
    if [[ $fail_count -gt 0 ]]; then
        echo -e "${RED}失败: $fail_count${NC}"
    fi
    echo ""
    
    # 显示输出目录
    if [[ $success_count -gt 0 ]]; then
        echo -e "${GREEN}编译输出目录: $BUILD_DIR${NC}"
    fi
    
    return $fail_count
}

# 主函数
main() {
    local SOURCE_DIR="$DEFAULT_SOURCE_DIR"
    local BUILD_DIR="$DEFAULT_BUILD_DIR"
    local themes=()
    local list_only=false
    
    # 解析命令行参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            -s|--source)
                SOURCE_DIR="$2"
                shift 2
                ;;
            -d|--dest)
                BUILD_DIR="$2"
                shift 2
                ;;
            -c|--compiler)
                COMPILER="$2"
                shift 2
                ;;
            -l|--list)
                list_only=true
                shift
                ;;
            -h|--help)
                show_usage
                exit 0
                ;;
            -*)
                echo -e "${RED}错误: 未知选项 $1${NC}"
                show_usage
                exit 1
                ;;
            *)
                themes+=("$1")
                shift
                ;;
        esac
    done
    
    # 显示配置信息
    echo -e "${BLUE}=== RIME 主题批量编译 ===${NC}"
    echo -e "源目录: $SOURCE_DIR"
    echo -e "输出目录: $BUILD_DIR"
    echo -e "编译器: $COMPILER"
    echo ""
    
    # 检查源目录是否存在
    if [[ ! -d "$SOURCE_DIR" ]]; then
        echo -e "${RED}错误: 源目录不存在: $SOURCE_DIR${NC}"
        exit 1
    fi
    
    # 如果只是列出主题
    if [[ "$list_only" == true ]]; then
        list_themes
        exit 0
    fi
    
    # 检查编译器
    check_compiler
    
    # 编译主题
    compile_themes "${themes[@]}"
    
    exit $?
}

# 运行主函数
main "$@" 