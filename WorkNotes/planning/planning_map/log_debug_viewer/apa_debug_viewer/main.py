#!/usr/bin/env python3
"""
APA Debug Viewer - 自动泊车日志调试可视化工具

用于分析 APA 日志数据，支持：
  - 建图 (Mapping)
  - 融合 (Fusion)
  - 边界 (Boundary)
  - 规划 (Planning)
  - 泊出 (ParkOut)

用法:
    python main.py <log_file>
    python main.py <log_file> --gui
    python main.py <log_file> --interactive
    python main.py <log_file> --output <dir>
"""

import argparse
import sys
from pathlib import Path

# 确保项目根目录在 sys.path 中
_project_root = Path(__file__).resolve().parent
if str(_project_root) not in sys.path:
    sys.path.insert(0, str(_project_root))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="APA Debug Viewer - 自动泊车日志调试可视化工具",
    )
    parser.add_argument(
        "log_file",
        type=str,
        nargs="?",
        help="APA 日志文件路径",
    )
    parser.add_argument(
        "--interactive", "-i",
        action="store_true",
        help="以交互模式运行",
    )
    parser.add_argument(
        "--output", "-o",
        type=str,
        default="",
        help="输出目录（用于保存渲染结果）",
    )
    parser.add_argument(
        "--frame", "-f",
        type=int,
        default=None,
        help="指定显示第 N 帧 (1-based)",
    )
    parser.add_argument(
        "--list-frames",
        action="store_true",
        help="列出所有帧的概要信息",
    )
    parser.add_argument(
        "--gui", "-g",
        action="store_true",
        help="启动 GUI 交互模式（支持拖动、缩放、方向键导航）",
    )
    return parser.parse_args()


def main():
    args = parse_args()

    if not args.log_file:
        print("请指定日志文件路径")
        print("用法: python main.py <log_file>")
        sys.exit(1)

    log_path = Path(args.log_file)
    if not log_path.exists():
        print(f"文件不存在: {log_path}")
        sys.exit(1)

    from viewer.app import App

    app = App()

    print(f"正在加载日志: {log_path}")
    app.load_log(log_path)

    if args.list_frames:
        print("\n--- Frame List ---")
        for i, frame in enumerate(app.timeline._frames):
            info = app.inspector.inspect(frame)
            print(f"  #{frame.seq_num}: pose={info['current_pose']}, "
                  f"fusion={len(info['fusion_stages'])}, "
                  f"boundary={len(info['boundaries'])}")
        return

    if args.frame is not None:
        target = args.frame
        frame = app.timeline.go_to_seq(target) or app.timeline.go_to(target - 1)
        if frame:
            app.show_current()
        else:
            print(f"帧 #{target} 不存在")
        return

    if args.gui:
        print("\n启动 GUI 模式（拖动/缩放/方向键导航）...")
        app.run_gui()
    elif args.interactive:
        print("\n启动交互模式...")
        app.run_interactive()
    else:
        # 默认显示（已在 load_log 中自动跳到有效帧）
        app.show_current()

        if args.output:
            output_dir = Path(args.output)
            output_dir.mkdir(parents=True, exist_ok=True)


if __name__ == "__main__":
    main()
