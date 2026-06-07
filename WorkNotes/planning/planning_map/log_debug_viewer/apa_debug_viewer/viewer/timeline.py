from model.frame import FrameData


class Timeline:
    """
    时间轴管理器
    支持 Frame 编号、跳转、播放控制
    """

    def __init__(self):
        self._frames: list[FrameData] = []
        self._current_index: int = -1

    @property
    def total_frames(self) -> int:
        """总帧数"""
        return len(self._frames)

    @property
    def current_index(self) -> int:
        """当前帧索引 (0-based)"""
        return self._current_index

    @property
    def current_frame(self) -> FrameData | None:
        """当前帧数据"""
        if 0 <= self._current_index < len(self._frames):
            return self._frames[self._current_index]
        return None

    @property
    def has_previous(self) -> bool:
        """是否有上一帧"""
        return self._current_index > 0

    @property
    def has_next(self) -> bool:
        """是否有下一帧"""
        return self._current_index < len(self._frames) - 1

    def load(self, frames: list[FrameData]) -> None:
        """加载帧列表"""
        self._frames = list(frames)
        self._current_index = 0 if frames else -1

    def go_to(self, index: int) -> FrameData | None:
        """跳转到指定索引帧"""
        if 0 <= index < len(self._frames):
            self._current_index = index
            return self._frames[index]
        return None

    def go_to_seq(self, seq_num: int) -> FrameData | None:
        """按 seq_num 跳转"""
        for i, frame in enumerate(self._frames):
            if frame.seq_num == seq_num:
                self._current_index = i
                return frame
        return None

    def next(self) -> FrameData | None:
        """下一帧"""
        return self.go_to(self._current_index + 1)

    def previous(self) -> FrameData | None:
        """上一帧"""
        return self.go_to(self._current_index - 1)

    def first(self) -> FrameData | None:
        """第一帧"""
        return self.go_to(0)

    def last(self) -> FrameData | None:
        """最后一帧"""
        return self.go_to(len(self._frames) - 1)

    def get_frame_indices(self) -> list[int]:
        """获取所有帧的序号"""
        return [f.seq_num for f in self._frames]

    def get_range(self, start: int, end: int) -> list[FrameData]:
        """获取范围内的帧"""
        return self._frames[max(0, start):min(len(self._frames), end)]
