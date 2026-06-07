"""解析器单元测试"""

import sys
from pathlib import Path

# 添加项目根目录到 sys.path
_project_root = Path(__file__).resolve().parent.parent
if str(_project_root) not in sys.path:
    sys.path.insert(0, str(_project_root))

from parser.frame_splitter import FrameSplitter
from parser.common import ParserUtils
from parser.fusion_parser import FusionParser
from parser.vehicle_parser import VehicleParser
from parser.boundary_parser import BoundaryParser
from model.point import Point

SAMPLE_REAL_FRAME = Path(__file__).parent / "sample_real_frame.log"


# ==================== 合成日志测试 ====================

SAMPLE_SYNTHETIC_LOG = """
Frame data start
Planning seq_num: ####[19301]####
==INPUTDATA==TOTALMAP==T(1234)==carpose: 1000, 2000, 0.5
current car position: (1000, 2000, 0.0)
Input end pose: (5000, 3000, 1.57)
==FSDFusionObj2Success==
=BSegData==(Num(2),0(100,200),1(300,400))
=NSegData==(Num(2),0(500,600),1(700,800))
=FusSegData==(FusNum(2),0(900,1000),1(1100,1200))
==PDCFusionSubLaneSuccess==
=BSegData==(Num(2),0(-100,-200),1(-300,-400))
=FusSegData==(FusNum(2),0(-500,-600),1(-700,-800))
==ParkOutSlotInfoFromTotalMap==First==SlotPt:0(1000,2000),1(3000,4000),2(5000,6000),3(7000,8000)==CurLabel(25)
Current frame end.

Frame data start
Planning seq_num: ####[19302]####
==INPUTDATA==TOTALMAP==T(5678)==carpose: 1100, 2100, 0.1
==FSDFusionObj1Success==
=BSegData==(Num(3),0(150,250),1(350,450),2(550,650))
=FusSegData==(FusNum(3),0(1150,1250),1(1350,1450),2(1550,1650))
==ParkOutSlotInfoFromTotalMap==First==SlotPt:0(2000,3000),1(4000,5000),2(6000,7000),3(8000,9000)==CurLabel(27)
Current frame end.
"""


def test_frame_splitter():
    splitter = FrameSplitter()
    frames = splitter.split(SAMPLE_SYNTHETIC_LOG)
    assert len(frames) == 2, f"期望 2 帧, 得到 {len(frames)}"
    print(f"[OK] FrameSplitter: {len(frames)} frames")


def test_frame_splitter_with_seq():
    splitter = FrameSplitter()
    frames = splitter.split_with_seq(SAMPLE_SYNTHETIC_LOG)
    assert len(frames) == 2
    seq0, _ = frames[0]
    seq1, _ = frames[1]
    assert seq0 == 19301, f"期望 seq=19301, 得到 {seq0}"
    assert seq1 == 19302, f"期望 seq=19302, 得到 {seq1}"
    print(f"[OK] FrameSplitter with seq: {seq0}, {seq1}")


def test_parse_inline_segment():
    line = "=BSegData==(Num(2),0(100,200),1(300,400))"
    points = ParserUtils.parse_inline_segment(line)
    assert len(points) == 2, f"期望 2 个点, 得到 {len(points)}"
    assert abs(points[0].x - 0.1) < 0.001
    assert abs(points[1].y - 0.4) < 0.001
    print(f"[OK] parse_inline_segment: {len(points)} points")


def test_parse_inline_with_trash():
    """测试带垃圾数据的 inline 解析"""
    line = "=BSegData==(Num(4),0(100,200),1(-nan,0.00),2(300,400),3(-13874180500409497287772864512.00,0.00))"
    points = ParserUtils.parse_inline_segment(line)
    # Num=4, 4个pair, idx1(nan)跳过, idx3(巨大值)跳过, 得到2个有效点
    assert len(points) == 2, f"期望 2 个有效点, 得到 {len(points)}"
    print(f"[OK] parse_inline_with_trash: {len(points)} valid points (trash filtered)")


def test_is_valid_point():
    import math
    assert ParserUtils.is_valid_point_value(100.0)
    assert not ParserUtils.is_valid_point_value(float('nan'))
    assert not ParserUtils.is_valid_point_value(float('inf'))
    assert not ParserUtils.is_valid_point_value(-1e20)
    print("[OK] is_valid_point_value")


def test_mm_to_m():
    assert abs(ParserUtils.mm_to_m(1000) - 1.0) < 0.001
    assert abs(ParserUtils.mm_to_m(2350) - 2.35) < 0.001
    print("[OK] ParserUtils.mm_to_m")


def test_full_parse_synthetic():
    from parser.frame_parser import FrameParser
    splitter = FrameSplitter()
    parser = FrameParser()
    frames = splitter.split_with_seq(SAMPLE_SYNTHETIC_LOG)
    for seq_num, frame_text in frames:
        frame = parser.parse(frame_text, seq_num=seq_num)
        assert frame.seq_num == seq_num
        print(f"  Frame #{seq_num}: pose={frame.current_pose}, "
              f"slot={len(frame.slot_pts)} pts, "
              f"fusion={len(frame.fusion_stages)} stages")
    print("[OK] Full parse synthetic OK")


# ==================== 真实日志测试 ====================

def test_real_frame_split():
    if not SAMPLE_REAL_FRAME.exists():
        print("[SKIP] real frame log not found")
        return
    text = SAMPLE_REAL_FRAME.read_text(encoding="utf-8", errors="replace")
    splitter = FrameSplitter()
    frames = splitter.split_with_seq(text)
    print(f"[INFO] Real log: {len(frames)} frames extracted")
    for seq, _ in frames[:3]:
        print(f"       seq_num={seq}")
    assert len(frames) > 0
    print(f"[OK] Real frame split: {len(frames)} frames")


def test_real_frame_parse():
    if not SAMPLE_REAL_FRAME.exists():
        print("[SKIP] real frame log not found")
        return
    from parser.frame_parser import FrameParser
    text = SAMPLE_REAL_FRAME.read_text(encoding="utf-8", errors="replace")
    splitter = FrameSplitter()
    parser = FrameParser()
    frames = splitter.split_with_seq(text)
    for seq_num, frame_text in frames[:3]:
        frame = parser.parse(frame_text, seq_num=seq_num)
        print(f"\n  --- Frame #{seq_num} ---")
        print(f"    pose: {frame.current_pose}")
        print(f"    goal: {frame.goal_pose}")
        print(f"    slot: {len(frame.slot_pts)} pts")
        print(f"    fusion stages: {len(frame.fusion_stages)}")
        for stage in frame.fusion_stages:
            b = stage.bseg.size if stage.bseg else 0
            n = stage.nseg.size if stage.nseg else 0
            f = stage.fusseg.size if stage.fusseg else 0
            print(f"      [{stage.name}] B={b} N={n} F={f}")
        print(f"    boundaries: {len(frame.boundaries)}")
        for b in frame.boundaries:
            print(f"      {b.name}: {b.size} pts")
    assert len(frames) > 0
    print(f"\n[OK] Real frame parse: {len(frames)} frames parsed")


def test_fusion_parser_real():
    if not SAMPLE_REAL_FRAME.exists():
        print("[SKIP] real frame log not found")
        return
    text = SAMPLE_REAL_FRAME.read_text(encoding="utf-8", errors="replace")
    splitter = FrameSplitter()
    frames = splitter.split(text)
    fusion_parser = FusionParser()
    total_stages = 0
    for i, frame_text in enumerate(frames[:3]):
        stages = fusion_parser.parse_all(frame_text)
        total_stages += len(stages)
        for stage in stages:
            b = stage.bseg.size if stage.bseg else 0
            n = stage.nseg.size if stage.nseg else 0
            f = stage.fusseg.size if stage.fusseg else 0
            print(f"  Frame {i+1}: {stage.name} B={b} N={n} F={f}")
    print(f"[OK] Fusion parser real: {total_stages} total stages (filtered)")


def test_vehicle_parser_real():
    if not SAMPLE_REAL_FRAME.exists():
        print("[SKIP] real frame log not found")
        return
    text = SAMPLE_REAL_FRAME.read_text(encoding="utf-8", errors="replace")
    splitter = FrameSplitter()
    frames = splitter.split(text)
    for i, frame_text in enumerate(frames[:3]):
        pose = VehicleParser.parse_current_pose(frame_text)
        goal = VehicleParser.parse_goal_pose(frame_text)
        slot = VehicleParser.parse_slot_points(frame_text)
        print(f"  Frame {i+1}: pose={pose}, goal={goal}, slot={len(slot)} pts")
    print(f"[OK] Vehicle parser real")


if __name__ == "__main__":
    test_is_valid_point()
    test_mm_to_m()
    test_parse_inline_segment()
    test_parse_inline_with_trash()
    test_frame_splitter()
    test_frame_splitter_with_seq()
    test_full_parse_synthetic()
    test_real_frame_split()
    test_fusion_parser_real()
    test_vehicle_parser_real()
    test_real_frame_parse()
    print("\n所有解析器测试通过!")
