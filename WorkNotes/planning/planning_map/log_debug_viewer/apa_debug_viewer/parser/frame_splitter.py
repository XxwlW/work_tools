import re


class FrameSplitter:
    """根据 'Current frame end.' 切分日志为多个 Frame 文本块"""

    FRAME_END_MARKER = "Current frame end."

    def split(self, text: str) -> list[str]:
        """将完整日志切分为 frame 文本块列表"""
        if not text or not text.strip():
            return []

        # 按 frame end marker 切分
        raw_frames = re.split(
            rf"\s*{re.escape(self.FRAME_END_MARKER)}\s*",
            text,
            flags=re.IGNORECASE,
        )

        frames: list[str] = []
        for raw in raw_frames:
            cleaned = raw.strip()
            if cleaned:
                frames.append(cleaned)

        return frames

    def split_with_seq(self, text: str) -> list[tuple[int, str]]:
        """
        切分并尝试提取 seq_num
        返回: [(seq_num, frame_text), ...]
        """
        raw_frames = self.split(text)
        result: list[tuple[int, str]] = []
        for i, frame_text in enumerate(raw_frames):
            seq = self._extract_seq_num(frame_text, i)
            result.append((seq, frame_text))
        return result

    @staticmethod
    def _extract_seq_num(frame_text: str, fallback: int) -> int:
        """尝试从 frame 文本中提取 seq_num"""
        # 真实格式: "Planning seq_num: ####[19301]####"
        patterns = [
            r"Planning\s+seq_num[:\s=]+\#*\s*\[?(\d+)\]?",
            r"Planning\s+seq_num[:\s=]+(\d+)",
            r"seq_num[:\s=]+(\d+)",
            r"SeqNum[:\s=]+(\d+)",
            r"seq\s*num[:\s=]+(\d+)",
        ]
        for pattern in patterns:
            match = re.search(pattern, frame_text, re.IGNORECASE)
            if match:
                return int(match.group(1))
        return fallback + 1
