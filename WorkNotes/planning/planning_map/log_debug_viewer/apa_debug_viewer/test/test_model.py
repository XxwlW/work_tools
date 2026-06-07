"""数据模型单元测试"""

import sys
from pathlib import Path

# 添加项目根目录到 sys.path
_project_root = Path(__file__).resolve().parent.parent
if str(_project_root) not in sys.path:
    sys.path.insert(0, str(_project_root))

from model.point import Point
from model.pose import Pose
from model.segment import Segment
from model.fusion import FusionStage
from model.boundary import Boundary
from model.frame import FrameData, SlotInfo


def test_point():
    p = Point(x=1.0, y=2.0)
    assert p.x == 1.0
    assert p.y == 2.0
    print("[OK] Point")


def test_pose():
    p = Pose(x=1.0, y=2.0, yaw=0.5)
    assert p.x == 1.0
    assert p.y == 2.0
    assert p.yaw == 0.5
    print("[OK] Pose")


def test_segment():
    s = Segment(points=[Point(1, 2), Point(3, 4)])
    assert s.size == 2
    assert not s.is_empty
    assert Segment().is_empty
    print("[OK] Segment")


def test_fusion_stage():
    stage = FusionStage(name="PDCFusionSubLaneSuccess")
    assert stage.name == "PDCFusionSubLaneSuccess"
    assert not stage.has_data

    stage.bseg = Segment(points=[Point(1, 2)])
    assert stage.has_data
    print("[OK] FusionStage")


def test_boundary():
    b = Boundary(name="MainBoundary", points=[Point(0, 0), Point(1, 1)])
    assert b.size == 2
    assert not b.is_empty
    print("[OK] Boundary")


def test_frame_data():
    frame = FrameData(seq_num=1)
    assert frame.seq_num == 1
    assert not frame.has_fusion_data
    assert not frame.has_boundary_data
    assert not frame.has_pdc_data

    frame.left_pdc = [Point(1, 2)]
    assert frame.has_pdc_data
    print("[OK] FrameData")


def test_slot_info():
    from model.point import Point
    si = SlotInfo(label=27, corner_pts=[Point(1, 2), Point(3, 4)])
    assert si.label == 27
    assert len(si.corner_pts) == 2
    print("[OK] SlotInfo")


if __name__ == "__main__":
    test_point()
    test_pose()
    test_segment()
    test_fusion_stage()
    test_boundary()
    test_frame_data()
    test_slot_info()
    print("\n所有模型测试通过!")
