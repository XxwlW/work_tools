from model.fusion import FusionStage
from model.segment import Segment
from .common import ParserUtils


class FusionParser:
    """
    解析 Fusion 相关数据。

    真实日志格式（INLINE 单行）:
        ==FSDFusionObj2Success==
        =BSegData==(Num(2),0(x0,y0),1(x1,y1),...)
        =NSegData==(Num(7),0(x0,y0),1(x1,y1),...)
        =FusLineIndex==(...)
        =Fusiondebug==(...)
        =FusSegData==(FusNum(5),0(x0,y0),1(x1,y1),...)

    支持以下 Stage markers（按解析顺序）:
        - FSDFusionSubLaneSuccess
        - FSDFusionObj1Success
        - FSDFusionObj2Success
        - SDGFusionObj1InnerBorderSuccess
        - SDGFusionObj1OuterBorderSuccess
        - SDGFusionObj2OuterBorderSuccess
        - PDCFusionObj1Success
        - PDCFusionObj2Success
        - PDCFusionSubLaneSuccess
    """

    STAGE_MARKERS = [
        "FSDFusionSubLaneSuccess",
        "FSDFusionObj1Success",
        "FSDFusionObj2Success",
        "SDGFusionObj1InnerBorderSuccess",
        "SDGFusionObj1OuterBorderSuccess",
        "SDGFusionObj2OuterBorderSuccess",
        "PDCFusionObj1Success",
        "PDCFusionObj2Success",
        "PDCFusionSubLaneSuccess",
    ]

    # 每个 Stage 内各 segment 的解析顺序
    SEGMENT_ORDER = ["BSegData", "NSegData", "FusSegData"]

    def parse_all(self, frame_text: str) -> list[FusionStage]:
        """解析 frame 文本中所有 FusionStage"""
        stages: list[FusionStage] = []
        lines = frame_text.split("\n")

        current_stage: FusionStage | None = None
        current_bseg: list[str] = []
        current_nseg: list[str] = []
        current_fusseg: list[str] = []
        last_seg_type: str | None = None

        for line in lines:
            stripped = line.strip()
            if not stripped:
                continue

            # 检查是否遇到新的 stage marker
            stage_matched = self._is_stage_marker(stripped)

            if stage_matched:
                # 保存上一个 stage
                if current_stage is not None:
                    self._finalize_stage(current_stage, current_bseg, current_nseg, current_fusseg)
                    if current_stage.has_data:
                        stages.append(current_stage)

                # 开始新 stage
                current_stage = FusionStage(name=stage_matched)
                current_bseg, current_nseg, current_fusseg = [], [], []
                last_seg_type = None
                continue

            if current_stage is None:
                continue

            # 解析 BSegData/NSegData/FusSegData 行
            seg_type = self._match_segment_type(stripped)
            if seg_type == "BSegData":
                current_bseg.append(stripped)
                last_seg_type = "BSegData"
            elif seg_type == "NSegData":
                current_nseg.append(stripped)
                last_seg_type = "NSegData"
            elif seg_type == "FusSegData":
                current_fusseg.append(stripped)
                last_seg_type = "FusSegData"
            elif "FusLineIndex" in stripped or "Fusiondebug" in stripped:
                # 辅助信息，目前跳过
                if current_stage is not None:
                    if current_stage.debug is None:
                        current_stage.debug = {}
                    current_stage.debug[stripped.split("==")[0].strip("=")] = stripped

        # 保存最后一个 stage
        if current_stage is not None:
            self._finalize_stage(current_stage, current_bseg, current_nseg, current_fusseg)
            if current_stage.has_data:
                stages.append(current_stage)

        return stages

    def _finalize_stage(
        self, stage: FusionStage,
        bseg_lines: list[str], nseg_lines: list[str], fusseg_lines: list[str],
    ) -> None:
        """将收集的行数据转换为 Segment"""
        if bseg_lines:
            points = self._parse_inline_points(bseg_lines[0])
            if points:
                stage.bseg = Segment(points=points)
        if nseg_lines:
            points = self._parse_inline_points(nseg_lines[0])
            if points:
                stage.nseg = Segment(points=points)
        if fusseg_lines:
            points = self._parse_inline_points(fusseg_lines[0])
            if points:
                stage.fusseg = Segment(points=points)

    @staticmethod
    def _parse_inline_points(line: str) -> list:
        """解析 inline 格式的点数据行"""
        return ParserUtils.parse_inline_segment(line)

    @staticmethod
    def _is_stage_marker(stripped: str) -> str | None:
        """检查是否匹配 stage marker，返回匹配的 stage 名称"""
        for marker in FusionParser.STAGE_MARKERS:
            if f"=={marker}==" in stripped:
                return marker
        return None

    @staticmethod
    def _match_segment_type(stripped: str) -> str | None:
        """匹配 BSegData/NSegData/FusSegData"""
        for seg_type in FusionParser.SEGMENT_ORDER:
            if stripped.startswith(f"={seg_type}==") or f"={seg_type}==" in stripped:
                return seg_type
        # 也匹配不带等号前缀的格式
        for seg_type in FusionParser.SEGMENT_ORDER:
            if stripped.startswith(seg_type):
                return seg_type
        return None
