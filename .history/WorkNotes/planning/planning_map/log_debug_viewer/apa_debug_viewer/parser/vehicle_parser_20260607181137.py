import re
from model.point import Point
from model.pose import Pose
from .common import ParserUtils


class VehicleParser:
    """
    解析车辆相关数据：
      - 当前位姿 (current_pose)
      - 目标位姿 (goal_pose)
      - 停车位角点 (slot_points)
      - PDC 数据 (left_pdc / right_pdc)
    """

    @staticmethod
    def parse_current_pose(frame_text: str) -> Pose | None:
        """解析当前车辆位姿"""
        # 优先从 ==INPUTDATA== 行解析 carpose（真实日志）
        for line in frame_text.split("\n"):
            stripped = line.strip()
            if "==INPUTDATA==" in stripped and "carpose:" in stripped:
                m = re.search(r"carpose:\s*([-+]?\d+\.?\d*),([-+]?\d+\.?\d*),([-+]?\d+\.?\d*)", stripped)
                if m:
                    x, y, yaw = float(m.group(1)), float(m.group(2)), float(m.group(3))
                    if ParserUtils.is_valid_point_value(x) and ParserUtils.is_valid_point_value(y):
                        return Pose(x=ParserUtils.mm_to_m(x), y=ParserUtils.mm_to_m(y), yaw=yaw)
            if stripped.startswith("carpose:") and "==" not in stripped:
                nums = re.findall(r"[-+]?\d+\.?\d*", stripped.split("carpose:", 1)[1])
                if len(nums) >= 3:
                    x, y, yaw = float(nums[0]), float(nums[1]), float(nums[2])
                    if ParserUtils.is_valid_point_value(x) and ParserUtils.is_valid_point_value(y):
                        if abs(x) > 1e6:
                            return Pose(x=ParserUtils.mm_to_m(x), y=ParserUtils.mm_to_m(y), yaw=yaw)
                        else:
                            return Pose(x=x, y=y, yaw=yaw)
        return None

    @staticmethod
    def parse_goal_pose(frame_text: str) -> Pose | None:
        """解析目标位姿 (Input end pose)"""
        for line in frame_text.split("\n"):
            stripped = line.strip()
            if "Input end pose:" in stripped:
                m = re.search(r"Input end pose:\s*([-+]?\d+\.?\d*),\s*([-+]?\d+\.?\d*),\s*([-+]?\d+\.?\d*)\s*\((deg)\)", stripped)
                if m:
                    x, y, angle = float(m.group(1)), float(m.group(2)), float(m.group(3))
                    if ParserUtils.is_valid_point_value(x) and ParserUtils.is_valid_point_value(y):
                        import math
                        yaw_rad = angle * math.pi / 180.0
                        return Pose(x=x, y=y, yaw=yaw_rad)
                m2 = re.search(r"Input end pose:\s*\(?\s*([-+]?\d+\.?\d*),\s*([-+]?\d+\.?\d*),\s*([-+]?\d+\.?\d*)\)?", stripped)
                if m2:
                    x, y, yaw = float(m2.group(1)), float(m2.group(2)), float(m2.group(3))
                    if ParserUtils.is_valid_point_value(x) and ParserUtils.is_valid_point_value(y):
                        if abs(x) > 1e6:
                            return Pose(x=ParserUtils.mm_to_m(x), y=ParserUtils.mm_to_m(y), yaw=yaw)
                        else:
                            return Pose(x=x, y=y, yaw=yaw)
        return None

    @staticmethod
    def parse_slot_points(frame_text: str) -> list[Point]:
        """解析停车位角点 SlotPt:0(x,y),1(x,y),2(x,y),3(x,y)"""
        points: list[Point] = []
        for line in frame_text.split("\n"):
            if "SlotPt:" not in line:
                continue
            pts = re.findall(
                r"SlotPt\[\d+\]\s*\(\s*([-+]?\d+\.?\d*)\s*,\s*([-+]?\d+\.?\d*)\s*\)",
                line,
            )
            for x_str, y_str in pts:
                x, y = float(x_str), float(y_str)
                if ParserUtils.is_valid_point_value(x) and ParserUtils.is_valid_point_value(y):
                    if abs(x) > 1e6:
                        points.append(Point(x=ParserUtils.mm_to_m(x), y=ParserUtils.mm_to_m(y)))
                    else:
                        points.append(Point(x=x, y=y))
        return points

    @staticmethod
    def parse_pdc_data(frame_text: str) -> tuple[list[Point], list[Point]]:
        """
        解析 PDC 数据。
        从 INPUTDATA 行中提取 p1, p7, p13 等标记点。
        p1/p7 归类为 left_pdc，p13 归类为 right_pdc。
        """
        left_pdc: list[Point] = []
        right_pdc: list[Point] = []

        for line in frame_text.split("\n"):
            if "==INPUTDATA==" not in line:
                continue
            p_matches = re.findall(
                r"(p\d+)\s*\(\s*([-+]?\d+\.?\d*)\s*,\s*([-+]?\d+\.?\d*)\s*\)", line
            )
            for name, x_str, y_str in p_matches:
                x, y = float(x_str), float(y_str)
                if not ParserUtils.is_valid_point_value(x) or not ParserUtils.is_valid_point_value(y):
                    continue
                pt = Point(x=ParserUtils.mm_to_m(x), y=ParserUtils.mm_to_m(y))
                if name in ("p1", "p7"):
                    left_pdc.append(pt)
                elif name == "p13":
                    right_pdc.append(pt)

        return left_pdc, right_pdc
