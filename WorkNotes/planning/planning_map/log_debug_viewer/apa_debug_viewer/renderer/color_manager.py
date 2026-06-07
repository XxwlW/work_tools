class ColorManager:
    """统一管理各图层的颜色"""

    _COLORS = {
        "BSeg":        "#1f77b4",  # 蓝色
        "NSeg":        "#ff7f0e",  # 橙色
        "FusSeg":      "#d62728",  # 红色
        "Boundary":    "#000000",  # 黑色
        "SubBoundary": "#888888",  # 灰色
        "Slot":        "#2ca02c",  # 绿色
        "LeftPDC":     "#17becf",  # 青色
        "RightPDC":    "#e377c2",  # 粉色
        "CurrentVehicle": "#1f77b4",  # 蓝色
        "GoalVehicle": "#2ca02c",     # 绿色
        "Path":        "#bcbd22",  # 黄绿色
        "Default":     "#7f7f7f",  # 灰色
    }

    @classmethod
    def get(cls, layer_name: str) -> str:
        """获取图层对应的颜色"""
        return cls._COLORS.get(layer_name, cls._COLORS["Default"])

    @classmethod
    def get_all_colors(cls) -> dict[str, str]:
        """获取所有颜色映射"""
        return dict(cls._COLORS)

    @classmethod
    def register(cls, layer_name: str, color_hex: str) -> None:
        """注册/覆盖图层颜色"""
        cls._COLORS[layer_name] = color_hex
