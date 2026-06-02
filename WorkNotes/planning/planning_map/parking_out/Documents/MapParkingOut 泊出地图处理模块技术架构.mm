<map version="1.0.1">
<!-- FreeMind 格式导出 by NotebookLM Export Tool -->
<node TEXT="MapParkingOut 泊出地图处理模块">
<node TEXT="系统架构与接口" POSITION="right">
<node TEXT="核心头文件 (Map.h, AlgCom.h, MapParkingOut.h)" />
<node TEXT="核心任务函数 APAMap_ParkingOutTask" />
<node TEXT="数据交换接口 (someip/planning_data)" />
</node>
<node TEXT="泊出任务流程" POSITION="left">
<node TEXT="状态校验 (APAstate, APARunningstate)" />
<node TEXT="初始化与调试 APAMap_ParkingOutDebugInit" />
<node TEXT="计算流程 (CalMapSlotPar -&gt; CalSlotInfo -&gt; CalMapInfo)" />
<node TEXT="有效性检查 APAMap_ParkingOutCheckIfCarPosIsValid" />
</node>
<node TEXT="车位参数计算 (SlotPar)" POSITION="right">
<node TEXT="泊出模式 (平行, 头进, 尾进)" />
<node TEXT="车位侧向判断 (左侧/右侧)" />
<node TEXT="车位边界点确定 (Obj1Pt, Obj2Pt)" />
<node TEXT="车位尺寸计算 (SlotLen, SlotDepth)" />
</node>
<node TEXT="感知数据融合" POSITION="left">
<node TEXT="超声波传感器融合 (USS/PDC)" />
<node TEXT="视觉车道线融合 (LaneLine)" />
<node TEXT="全景FSD数据融合 (TopViewFSD)" />
<node TEXT="目标检测融合 (OD/SquareColumn/Curb)" />
</node>
<node TEXT="终点位置管理 (EndPos)" POSITION="right">
<node TEXT="终点生成 (ParkingOutSetEndCarPos)" />
<node TEXT="UWB定位辅助 (SUPPORT_PARKING_OUT_UWB)" />
<node TEXT="入侵冲突处理 (SeizeEndCarPosInfo)" />
<node TEXT="终点居中策略 (CenterEndCarPosInfo)" />
</node>
<node TEXT="地图边界更新 (Boundary)" POSITION="left">
<node TEXT="主边界处理 (MainBoudary)" />
<node TEXT="子边界处理 (SubBoundary)" />
<node TEXT="平滑处理 APAMap_SmoothMapBoundary" />
<node TEXT="锚点转换逻辑 (AfterNewAnchorPointFlag)" />
</node>
<node TEXT="特殊场景处理" POSITION="right">
<node TEXT="斜列/阶梯车位 (LadderSlot/AngledSlot)" />
<node TEXT="狭窄通道处理 (WideChannelforParallelFlag)" />
<node TEXT="电子围栏生成 (ElectrFenceMapBulid)" />
</node>
</node>
</map>