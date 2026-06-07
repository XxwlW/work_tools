"""几何工具单元测试"""

import sys
import math
from pathlib import Path

_project_root = Path(__file__).resolve().parent.parent
if str(_project_root) not in sys.path:
    sys.path.insert(0, str(_project_root))

from model.pose import Pose
from model.point import Point
from utils.geometry import (
    build_vehicle_polygon,
    compute_distance,
    compute_bearing,
)


def test_build_vehicle_polygon():
    pose = Pose(x=0, y=0, yaw=0)
    polygon = build_vehicle_polygon(pose, length=4.0, width=2.0)

    assert len(polygon) == 5, f"期望 5 个点, 得到 {len(polygon)}"
    # 验证闭合
    assert abs(polygon[0].x - polygon[-1].x) < 0.001
    assert abs(polygon[0].y - polygon[-1].y) < 0.001

    print(f"[OK] build_vehicle_polygon: {len(polygon)} points")


def test_rotated_vehicle():
    pose = Pose(x=0, y=0, yaw=math.pi / 2)
    polygon = build_vehicle_polygon(pose, length=4.0, width=2.0)

    # 车头朝 Y+，前边两个点 y 应该为正
    front_y = polygon[0].y
    back_y = polygon[2].y
    assert front_y > back_y, f"前 y={front_y} 应 > 后 y={back_y}"
    print(f"[OK] Rotated vehicle: front_y={front_y:.2f}, back_y={back_y:.2f}")


def test_compute_distance():
    d = compute_distance(Point(0, 0), Point(3, 4))
    assert abs(d - 5.0) < 0.001
    print(f"[OK] compute_distance: {d}")


def test_compute_bearing():
    b = compute_bearing(Point(0, 0), Point(1, 1))
    assert abs(b - math.pi / 4) < 0.001
    print(f"[OK] compute_bearing: {b:.3f}")


if __name__ == "__main__":
    test_build_vehicle_polygon()
    test_rotated_vehicle()
    test_compute_distance()
    test_compute_bearing()
    print("\n所有几何测试通过!")
