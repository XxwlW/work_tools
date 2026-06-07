import math
from model.point import Point
from model.pose import Pose

# 默认车辆尺寸 (meter)
DEFAULT_VEHICLE_LENGTH = 4.8
DEFAULT_VEHICLE_WIDTH = 1.9


def build_vehicle_polygon(
    pose: Pose,
    length: float = DEFAULT_VEHICLE_LENGTH,
    width: float = DEFAULT_VEHICLE_WIDTH,
) -> list[Point]:
    """
    根据车辆位姿构建车辆矩形多边形
    返回 5 个点（首尾相同，形成闭合）
    """
    cos_yaw = math.cos(pose.yaw)
    sin_yaw = math.sin(pose.yaw)

    hw = width / 2.0
    hl = length / 2.0

    # 四个角相对于中心的偏移
    corners = [
        (hl, hw),   # 右前
        (hl, -hw),  # 左前
        (-hl, -hw), # 左后
        (-hl, hw),  # 右后
    ]

    polygon = []
    for dx, dy in corners:
        wx = dx * cos_yaw - dy * sin_yaw + pose.x
        wy = dx * sin_yaw + dy * cos_yaw + pose.y
        polygon.append(Point(x=wx, y=wy))

    # 添加第一个点以闭合
    polygon.append(polygon[0])

    return polygon


def compute_distance(p1: Point, p2: Point) -> float:
    """计算两点间距离"""
    dx = p1.x - p2.x
    dy = p1.y - p2.y
    return math.sqrt(dx * dx + dy * dy)


def compute_bearing(p1: Point, p2: Point) -> float:
    """计算从 p1 到 p2 的方位角 (弧度)"""
    return math.atan2(p2.y - p1.y, p2.x - p1.x)


def interpolate_points(p1: Point, p2: Point, num_points: int) -> list[Point]:
    """在两点之间插值"""
    if num_points < 2:
        return [p1, p2]
    points = []
    for i in range(num_points):
        t = i / (num_points - 1)
        x = p1.x + (p2.x - p1.x) * t
        y = p1.y + (p2.y - p1.y) * t
        points.append(Point(x=x, y=y))
    return points
