import re
import math
from model.point import Point


class ParserUtils:
    """解析通用工具"""

    # mm -> m 转换因子
    MM_TO_M = 0.001

    # 有效坐标范围 (mm): 超出此范围视为无效数据
    MAX_VALID_COORD_MM = 1000000.0
    # 校验值: 巨大浮点数（未初始化内存）阈值
    MAX_SANITY_VALUE = 1e12

    @staticmethod
    def mm_to_m(value: float) -> float:
        """毫米转米"""
        return value * ParserUtils.MM_TO_M

    @staticmethod
    def is_valid_point_value(value: float) -> bool:
        """检查坐标值是否有效（非NaN、非Inf、非巨大垃圾值）"""
        if math.isnan(value) or math.isinf(value):
            return False
        if abs(value) > ParserUtils.MAX_SANITY_VALUE:
            return False
        return True

    @staticmethod
    def parse_point_line(line: str) -> Point | None:
        """从日志行解析 Point，支持格式: (x, y) 或 x, y 或 x y"""
        line = line.strip()
        nums = [float(x) for x in re.findall(r"[-+]?\d+\.?\d*(?:[eE][-+]?\d+)?", line)]
        if len(nums) >= 2:
            x, y = nums[0], nums[1]
            if ParserUtils.is_valid_point_value(x) and ParserUtils.is_valid_point_value(y):
                # 如果坐标绝对值非常大（>1e6），可能是局部坐标系中的 mm 值
                return Point(
                    x=ParserUtils.mm_to_m(x),
                    y=ParserUtils.mm_to_m(y),
                )
        return None

    @staticmethod
    def parse_inline_segment(line: str) -> list[Point]:
        """
        解析 INLINE 格式的点数据:
        =BSegData==(Num(2),0(x0,y0),1(x1,y1),...,19(x19,y19))

        只提取前 Num(N) 个有效点，过滤掉垃圾数据
        """
        # 提取 Num(N) 中的 N
        num_match = re.search(r'Num\s*\((\d+)\)', line)
        if not num_match:
            return []
        expected_count = int(num_match.group(1))
        if expected_count <= 0 or expected_count > 50:
            return []

        # 提取所有 idx(x,y) 对
        points: list[Point] = []
        # 匹配模式: idx(x,y) 其中 idx=0~19
        pairs = re.findall(r'\d+\s*\(\s*([-+]?\d+\.?\d*(?:[eE][-+]?\d+)?)\s*,\s*([-+]?\d+\.?\d*(?:[eE][-+]?\d+)?)\s*\)', line)
        for x_str, y_str in pairs[:expected_count]:
            x, y = float(x_str), float(y_str)
            if ParserUtils.is_valid_point_value(x) and ParserUtils.is_valid_point_value(y):
                points.append(Point(
                    x=ParserUtils.mm_to_m(x),
                    y=ParserUtils.mm_to_m(y),
                ))

        return points

    @staticmethod
    def parse_points_block(lines: list[str]) -> list[Point]:
        """解析连续的多行点数据，每行一个点"""
        points: list[Point] = []
        for line in lines:
            pt = ParserUtils.parse_point_line(line)
            if pt is not None:
                points.append(pt)
        return points

    @staticmethod
    def extract_section_lines(
        text: str, start_marker: str, end_markers: list[str],
    ) -> list[str]:
        """从文本中提取两个标记之间的行"""
        lines = text.split("\n")
        in_section = False
        section_lines: list[str] = []
        for line in lines:
            stripped = line.strip()
            if start_marker in stripped:
                in_section = True
                continue
            if in_section:
                should_end = False
                for em in end_markers:
                    if em in stripped:
                        should_end = True
                        break
                if should_end:
                    break
                section_lines.append(line)
        return section_lines

    @staticmethod
    def extract_inline_sections(
        text: str, start_marker: str, end_markers: list[str],
    ) -> list[str]:
        """提取 start_marker 行之后直到遇到 end_markers 的行"""
        lines = text.split("\n")
        result: list[str] = []
        found = False
        for line in lines:
            stripped = line.strip()
            if start_marker in stripped:
                found = True
                continue
            if found:
                should_stop = False
                for em in end_markers:
                    if em in stripped:
                        should_stop = True
                        break
                if should_stop or not stripped:
                    break
                result.append(line)
        return result
