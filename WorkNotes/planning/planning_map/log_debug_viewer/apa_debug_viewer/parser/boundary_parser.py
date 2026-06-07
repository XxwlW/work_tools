import re
from model.boundary import Boundary
from .common import ParserUtils


class BoundaryParser:
    """
    解析 Boundary 相关数据。

    真实日志格式 (ParkingOut):
    - ==APAMap_ParkingOutCalBoundaryByParkOutInfo==...DefaulBordenObj1...
    - =BSegData==(...)  (来自 Fusion，包含边界点)
    - SlotBordPt: SlotBordPt[0](x,y), SlotBordPt[1](x,y)
    - main_boundary / sub_boundary (未来日志支持)
    """

    @staticmethod
    def parse_all(frame_text: str) -> list[Boundary]:
        """解析 frame 文本中所有边界数据"""
        boundaries: list[Boundary] = []

        # 1. SlotBordPt (停车位边界点)
        slot_boundary = BoundaryParser._parse_slot_border(frame_text)
        if slot_boundary is not None:
            boundaries.append(slot_boundary)

        # 2. main_boundary (未来日志)
        main_b = BoundaryParser._parse_main_boundary(frame_text)
        if main_b is not None:
            boundaries.append(main_b)

        # 3. sub_boundary (未来日志)
        sub_b = BoundaryParser._parse_sub_boundary(frame_text)
        if sub_b is not None:
            boundaries.append(sub_b)

        return boundaries

    @staticmethod
    def _parse_slot_border(text: str) -> Boundary | None:
        """解析 SlotBordPt: SlotBordPt[0](x,y), SlotBordPt[1](x,y)"""
        for line in text.split("\n"):
            if "SlotBordPt" in line and "(" in line:
                pts = re.findall(r'SlotBordPt\[\d+\]\s*\(\s*([-+]?\d+\.?\d*)\s*,\s*([-+]?\d+\.?\d*)\s*\)', line)
                if pts:
                    points = []
                    for x_str, y_str in pts:
                        x, y = float(x_str), float(y_str)
                        if ParserUtils.is_valid_point_value(x) and ParserUtils.is_valid_point_value(y):
                            points.append(ParserUtils.mm_to_m(x) if abs(x) < 1e8 else x)
                            # 修正: 使用 Point
                    from model.point import Point
                    valid_points = []
                    for x_str, y_str in pts:
                        x, y = float(x_str), float(y_str)
                        if ParserUtils.is_valid_point_value(x) and ParserUtils.is_valid_point_value(y):
                            valid_points.append(Point(
                                x=ParserUtils.mm_to_m(x),
                                y=ParserUtils.mm_to_m(y),
                            ))
                    if valid_points:
                        return Boundary(name="SlotBorder", points=valid_points)
        return None

    @staticmethod
    def _parse_main_boundary(text: str) -> Boundary | None:
        """解析 MainBoundary"""
        lines = ParserUtils.extract_section_lines(
            text, "main_boundary", ["sub_boundary", "Current frame end", "=="],
        )
        points = ParserUtils.parse_points_block(lines)
        if len(points) > 0:
            return Boundary(name="MainBoundary", points=points)
        return None

    @staticmethod
    def _parse_sub_boundary(text: str) -> Boundary | None:
        """解析 SubBoundary"""
        lines = ParserUtils.extract_section_lines(
            text, "sub_boundary", ["main_boundary", "Current frame end", "=="],
        )
        points = ParserUtils.parse_points_block(lines)
        if len(points) > 0:
            return Boundary(name="SubBoundary", points=points)
        return None
