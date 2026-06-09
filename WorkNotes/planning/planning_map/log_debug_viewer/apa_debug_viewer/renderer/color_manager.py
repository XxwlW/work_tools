class ColorManager:
    """统一管理各图层的颜色"""

    # 按数据来源分类的颜色
    _SOURCE_COLORS = {
        "PDC": "#00BFBF",   # 深青色 - PDC 传感器
        "FSD": "#2E5BFF",   # 蓝色 - 前进方向传感器
        "SDG": "#8B3EFF",   # 紫色 - 停车位检测
    }

    # 按点类型的基础颜色（BSeg=原始点, NSeg=新增点, FusSeg=融合点）
    _TYPE_COLORS = {
        "BSeg":   "#6495ED",  # 矢车菊蓝 - 原始边界点
        "NSeg":   "#FF8C00",  # 橙色 - 新增点
        "FusSeg": "#FF4040",  # 红色 - 融合结果
    }

    # 其他固定图层的颜色
    _LAYER_COLORS = {
        "Boundary":    "#000000",  # 黑色
        "SubBoundary": "#666666",  # 深灰
        "Slot":        "#32CD32",  # 绿色
        "LeftPDC":     "#00BFBF",  # 青色
        "RightPDC":    "#00BFBF",  # 青色
        "CurrentVehicle": "#2E5BFF",  # 蓝色
        "GoalVehicle": "#32CD32",     # 绿色
        "Path":        "#BCBD22",  # 黄绿色
        "Default":     "#7f7f7f",  # 灰色
    }

    # 组合颜色（source + type），优先匹配
    _COMBINED_COLORS: dict[str, str] = {}

    @classmethod
    def _build_combined_colors(cls) -> None:
        """构建所有 source+type 组合颜色"""
        for source, scolor in cls._SOURCE_COLORS.items():
            for ptype, tcolor in cls._TYPE_COLORS.items():
                key = f"{source}_{ptype}"
                cls._COMBINED_COLORS[key] = tcolor  # 用 type 颜色区分

    @classmethod
    def get(cls, layer_name: str, source: str = "", point_type: str = "") -> str:
        """获取颜色

        Args:
            layer_name: 图层名 (BSeg/NSeg/FusSeg/Boundary/Slot等)
            source: 数据来源 (PDC/FSD/SDG)
            point_type: 点类型 (BSeg/NSeg/FusSeg/PDC)
        """
        if source and point_type and point_type in cls._TYPE_COLORS:
            # 使用类型颜色（BSeg/NSeg/FusSeg 用各自类型颜色，不
            return cls._TYPE_COLORS[point_type]

        if layer_name in cls._LAYER_COLORS:
            return cls._LAYER_COLORS[layer_name]

        return cls._LAYER_COLORS.get("Default", "#7f7f7f")

    @classmethod
    def get_by_source(cls, source: str, point_type: str = "") -> str:
        """根据来源和类型获取颜色"""
        if point_type in cls._TYPE_COLORS:
            return cls._TYPE_COLORS[point_type]
        return cls._SOURCE_COLORS.get(source, "#7f7f7f")

    @classmethod
    def get_all_colors(cls) -> dict[str, str]:
        """获取所有颜色映射"""
        result = dict(cls._LAYER_COLORS)
        result.update(cls._SOURCE_COLORS)
        result.update(cls._TYPE_COLORS)
        return result

    @classmethod
    def register(cls, layer_name: str, color_hex: str) -> None:
        """注册/覆盖图层颜色"""
        cls._LAYER_COLORS[layer_name] = color_hex