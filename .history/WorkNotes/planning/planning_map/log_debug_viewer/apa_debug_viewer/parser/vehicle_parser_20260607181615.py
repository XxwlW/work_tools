import re
import math
from model.pose import Pose
from model.point import Point
from .common import ParserUtils


class VehicleParser:
    """
    解析车辆相关数据。

    真实日志格式:
    - carpose: "someip==rxdata==t:...==carpose: x,y,yaw"
    - INPUTDATA: "==INPUTDATA==TOTALMAP==...==carpose: x,y,yaw"
    - loc: "*****==loc(x, y, L.., heading)loc****"
    - Slot: "==ParkOutSlotInfoFromTotalMap==First==SlotPt:0(x0,y0),1(x1,y1),..."
    - PDC: "GetPDCInfoByParkMode==Left" (未来支持)
    """

    @staticmethod
    def parse_current_pose(frame_text: str) -> Pose | None:
        """解析当前车辆位置"""
        # 方式1: 从 INPUTDATA 行提取 carpose
        for line in frame_text.split("\n"):
            if "INPUTDATA" in line and "carpose:" in line:
                pose = VehicleParser._parse_carpose_line(line)
                if pose is not None:
                    return pose

        # 方式2: 从 someip rxdata 行提取 carpose
        for line in frame_text.split("\n"):
            if "someip==rxdata" in line and "carpose:" in line:
                pose = VehicleParser._parse_carpose_line(line)
                if pose is not None:
                    return pose

        # 方式3: 从 loc(...) 格式提取
        for line in frame_text.split("\n"):
            pose = VehicleParser._parse_loc_line(line)
            if pose is not None:
                return pose

        return None

    @staticmethod
    def _parse_carpose_line(line: str) -> Pose | None:
        """从 carpose: x,y,yaw 格式解析"""
        match = re.search(r'carpose:\s*([-+]?\d+\.?\d*(?:[eE][-+]?\d+)?)\s*,\s*([-+]?\d+\.?\d*(?:[eE][-+]?\d+)?)\s*,?\s*([-+]?\d+\.?\d*(?:[eE][-+]?\d+)?)?', line)
        if match:
            x = float(match.group(1))
            y = float(match.group(2))
            yaw = float(match.group(3)) if match.group(3) else 0.0
            if ParserUtils.is_valid_point_value(x) and ParserUtils.is_valid_point_value(y):
                return Pose(
                    x=ParserUtils.mm_to_m(x),
                    y=ParserUtils.mm_to_m(y),
                    yaw=yaw,
                )
        return None

    @staticmethod
    def _parse_loc_line(line: str) -> Pose | None:
        """从 *****==loc(x, y, L.., heading)loc**** 格式解析"""
        if "loc(" not in line or ")loc" not in line:
            return None
        match = re.search(r'loc\s*\(\s*([-+]?\d+\.?\d*)\s*,\s*([-+]?\d+\.?\d*)\s*,\s*L?\s*[-+]?\d+\.?\d*\s*,\s*([-+]?\d+\.?\d*)', line)
        if match:
            x = float(match.group(1))
            y = float(match.group(2))
            heading = float(match.group(3))
            if ParserUtils.is_valid_point_value(x) and ParserUtils.is_valid_point_value(y):
                return Pose(
                    x=ParserUtils.mm_to_m(x),
                    y=ParserUtils.mm_to_m(y),
                    yaw=math.radians(heading),
                )
        return None

    @staticmethod
    def parse_goal_pose(frame_text: str) -> Pose | None:
        """解析目标位姿"""
        # 从 ==APAMap_ParkingOutSetEndCarPosInOldCorSys== 中提取 EndPos
        for line in frame_text.split("\n"):
            if "EndPos" in line and "ParkingOutSetEndCarPos" in line:
                match = re.search(r'EndPos\s*\(\s*([-+]?\d+\.?\d*)\s*,\s*([-+]?\d+\.?\d*)\s*,?\s*([-+]?\d+\.?\d*)?', line)
                if match:
                    x = float(match.group(1))
                    y = float(match.group(2))
                    yaw = float(match.group(3)) if match.group(3) else 0.0
                    if ParserUtils.is_valid_point_value(x) and ParserUtils.is_valid_point_value(y):
                        return Pose(
                            x=ParserUtils.mm_to_m(x),
                            y=ParserUtils.mm_to_m(y),
                            yaw=yaw,
                        )
        return None

    @staticmethod
    def parse_slot_points(frame_text: str) -> list[Point]:
        """解析停车位角点"""
        all_points: list[Point] = []
        for line in frame_text.split("\n"):
            if "ParkOutSlotInfoFromTotalMap" in line and "SlotPt:" in line:
                # 格式: SlotPt:0(x0,y0),1(x1,y1),2(x2,y2),3(x3,y3)
                points = VehicleParser._parse_slot_pt_line(line)
                if points:
                    # 只取第一个有效的 SlotPt 数据组
                    if not all_points:
                        all_points = points
        return all_points

    @staticmethod
    def _parse_slot_pt_line(line: str) -> list[Point]:
        """解析 SlotPt:0(x0,y0),1(x1,y1),... 格式"""
        points: list[Point] = []
        # 匹配 idx(x,y) 对
        pairs = re.findall(r'\d+\s*\(\s*([-+]?\d+\.?\d*)\s*,\s*([-+]?\d+\.?\d*)\s*\)', line)
        for x_str, y_str in pairs:
            x, y = float(x_str), float(y_str)
            if ParserUtils.is_valid_point_value(x) and ParserUtils.is_valid_point_value(y):
                points.append(Point(
                    x=ParserUtils.mm_to_m(x),
                    y=ParserUtils.mm_to_m(y),
                ))
        return points

    @staticmethod
    def parse_pdc_data(frame_text: str) -> tuple[list[Point], list[Point]]:
        """解析 PDC 数据 (待实现，目前日志中未出现)"""
        return [], []

    @staticmethod
    def _extract_all_numbers(text: str) -> list[float]:
        """提取文本中所有数字"""
        return [float(x) for x in re.findall(r"[-+]?\d+\.?\d*(?:[eE][-+]?\d+)?", text)]


