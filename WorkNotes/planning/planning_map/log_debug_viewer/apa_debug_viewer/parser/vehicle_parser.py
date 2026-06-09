import re
from model.pose import Pose
from model.point import Point
from .common import ParserUtils


class VehicleParser:
    """
    解析车辆相关数据。

    坐标系说明：
    - 使用车位局部坐标系 (slot local coordinate)，以车位中心 (obj2) 为原点
    - 车辆在车位内时，坐标接近 (0, 0)
    - 单位：米 (meter)

    真实日志格式:
    - 车位内当前位置: "init_x/y transfered in slot coordinate: (x, y)"
    - 车位目标位置: "end_pose in slot_coor: x, y, z, yaw"
    - 停车位角点: "SlotPt:0(x0,y0),1(x1,y1),..."
    """

    @staticmethod
    def parse_current_pose(frame_text: str) -> Pose | None:
        """解析当前车辆位置

        优先从 app===APAMap=Output 的 CarPos 字段提取（单位已是米）
        格式: CarPos(x,y,yaw) 其中 yaw可能是弧度或度
        """
        import math

        # 从 app===APAMap=Output 提取 CarPos（世界坐标，米）
        for line in frame_text.split("\n"):
            if "app===APAMap=Output" in line and "CarPos(" in line:
                # 格式: CarPos(-2.261266,0.578133,1.417834)
                match = re.search(r'CarPos\s*\(\s*([-+]?\d+\.?\d*)\s*,\s*([-+]?\d+\.?\d*)\s*,\s*([-+]?\d+\.?\d*)\s*\)', line)
                if match:
                    x = float(match.group(1))
                    y = float(match.group(2))
                    yaw = float(match.group(3))
                    if ParserUtils.is_valid_point_value(x) and ParserUtils.is_valid_point_value(y):
                        # yaw 可能是弧度值（约1.4 rad ≈ 81°），如果是较大的弧度值转回度
                        if abs(yaw) > 10:  # 超过 10 的可能是度
                            yaw_deg = yaw
                            yaw = math.radians(yaw_deg)
                        elif abs(yaw) < 6:  # 小于 6 的弧度值保持不变
                            pass
                        return Pose(x=x, y=y, yaw=yaw)

        #备选：从 INPUTDATA 的 carpose 提取（世界坐标，毫米）
        for line in frame_text.split("\n"):
            if "INPUTDATA" in line and "carpose:" in line:
                pose = VehicleParser._parse_carpose_line(line)
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
                # 检查是否看起来像毫米值（绝对值 > 1000 可能是毫米）
                if abs(x) > 100 or abs(y) > 100:
                    # 假设是毫米，转换为米
                    x = ParserUtils.mm_to_m(x)
                    y = ParserUtils.mm_to_m(y)
                return Pose(x=x, y=y, yaw=yaw)
        return None

    @staticmethod
    def parse_goal_pose(frame_text: str) -> Pose | None:
        """解析目标位姿（使用车位局部坐标系）"""
        # 从 end_pose in slot_coor 提取
        for line in frame_text.split("\n"):
            # 格式: end_pose in slot_coor: 7.7225, -4.0360, 0.0000, 0.0000
            match = re.search(r'end_pose\s+in\s+slot_coor:\s*([-+]?\d+\.?\d*)\s*,\s*([-+]?\d+\.?\d*)\s*,\s*([-+]?\d+\.?\d*)?\s*,\s*([-+]?\d+\.?\d*)?', line)
            if match:
                x = float(match.group(1))
                y = float(match.group(2))
                # z = float(match.group(3)) if match.group(3) else 0.0  # 暂未使用
                yaw = float(match.group(4)) if match.group(4) else 0.0
                if ParserUtils.is_valid_point_value(x) and ParserUtils.is_valid_point_value(y):
                    return Pose(x=x, y=y, yaw=yaw)

        # 备选：从 ParkingOutSetEndCarPosInOldCorSys 提取
        for line in frame_text.split("\n"):
            if "EndPos" in line and "ParkingOutSetEndCarPos" in line:
                match = re.search(r'EndPos\s*\(\s*([-+]?\d+\.?\d*)\s*,\s*([-+]?\d+\.?\d*)\s*,?\s*([-+]?\d+\.?\d*)?', line)
                if match:
                    x = float(match.group(1))
                    y = float(match.group(2))
                    yaw = float(match.group(3)) if match.group(3) else 0.0
                    if ParserUtils.is_valid_point_value(x) and ParserUtils.is_valid_point_value(y):
                        return Pose(x=x, y=y, yaw=yaw)
        return None

    @staticmethod
    def parse_slot_points(frame_text: str) -> list[Point]:
        """解析停车位角点 SlotPt（世界坐标，毫米）

        格式: SlotPt:0(x0,y0),1(x1,y1),2(x2,y2),3(x3,y3)
        这些是世界坐标（毫米），需要转换为米
        只取第一个有效的 SlotPt 组（去重）
        """
        all_points: list[Point] = []

        for line in frame_text.split("\n"):
            # 匹配 ===SlotPt:0 开始的行（精确匹配）
            if "===SlotPt:" in line:
                points = VehicleParser._parse_slot_pt_line(line)
                if points and len(points) >= 4:
                    # 检查是否和已有点重复（距离很近的点认为是重复）
                    if not all_points:
                        all_points = points
                        break  # 只取第一个

        return all_points

    @staticmethod
    def _parse_slot_pt_line(line: str) -> list[Point]:
        """解析 SlotPt:0(x0,y0),1(x1,y1),... 格式

        坐标是毫米，转换为米
        """
        points: list[Point] = []
        pairs = re.findall(r'\d+\s*\(\s*([-+]?\d+\.?\d*)\s*,\s*([-+]?\d+\.?\d*)\s*\)', line)
        for x_str, y_str in pairs:
            x, y = float(x_str), float(y_str)
            if ParserUtils.is_valid_point_value(x) and ParserUtils.is_valid_point_value(y):
                # 毫米转米
                points.append(Point(x=ParserUtils.mm_to_m(x), y=ParserUtils.mm_to_m(y)))
        return points

    @staticmethod
    def parse_pdc_data(frame_text: str) -> tuple[list[Point], list[Point]]:
        """解析 PDC 数据 (待实现，目前日志中未出现)"""
        return [], []

    @staticmethod
    def _extract_all_numbers(text: str) -> list[float]:
        """提取文本中所有数字"""
        return [float(x) for x in re.findall(r"[-+]?\d+\.?\d*(?:[eE][-+]?\d+)?", text)]