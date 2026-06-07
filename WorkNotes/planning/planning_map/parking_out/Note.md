
# 相关常量/宏定义
## 泊车运行状态：
APARunningState
```
typedef enum
{
    APA_RUNNING_STATE_DEACTIVATED_E = 0,/**< 0 */                               // APA 运行状态：已停用
    APA_RUNNING_STATE_SEARCHING_SLOT_E,/**< 1 */                                // APA 运行状态：搜索车位中
    APA_RUNNING_STATE_SLOT_FOUND_E,/**< 2 */                                    // APA 运行状态：已找到车位
    APA_RUNNING_STATE_REQ_CONTROL_ACTUATOR_E,/**< 3 */                          // APA 运行状态：请求控制执行器
    APA_RUNNING_STATE_ACTUATOR_CONTROL_CONNECTING_E,/**< 4 */                   // APA 运行状态：执行器控制连接中
    APA_RUNNING_STATE_PARKING_FORWARD_FIRSTLY_E,/**< 5 */                       // APA 运行状态：首次向前泊车
    APA_RUNNING_STATE_PARKING_STEPN_E,/**< 6 */                                 // APA 运行状态：第N步泊车中
    APA_RUNNING_STATE_PARKING_END_POSITION_ACHIEVED_E,/**< 7 */                 // APA 运行状态：已到达泊车终点位置
    APA_RUNNING_STATE_PARKING_FAIL_E/**< 8 */                                   // APA 运行状态：泊车失败
} APADec_enmAPARunningStateDataType_t;
```


在自动泊车（APA）的底层地图与规划模块中，`APARunningstate` 就像是整个系统的**“换挡器（状态机）”**。地图模块本身并不控制车辆，但它必须像一个“聪明的观察者”一样，时刻监控 `APARunningstate` 的值，从而决定当前应该**“从零建图”**、**“局部修补边界”**还是**“停止更新清理内存”**。

结合您提供的状态枚举值与源码，以下是这些状态在代码核心位置的分布及作用逻辑详细拆解：

### 1. 状态：`APARunningstate == 0` (已停用)
*   **代码位置**：`APAMap_ParkingOutCalMapSlotPar()`。
*   **源码片段**：`if (APAMap_GInputData.ParkReqPar.APARunningstate == 0) { APAMAP_Resetlastreqcnt(); }`
*   **作用逻辑**：当系统处于 `APA_RUNNING_STATE_DEACTIVATED_E` (已停用) 时，说明当前没有任何泊车任务，或者任务已经被彻底取消。此时代码会调用 `APAMAP_Resetlastreqcnt()` 将上一次请求的计数器重置。这是为了防止系统开启下一次泊出任务时，错误地继承了上一局的旧指令计数，起到**“彻底清零打底”**的作用。

### 2. 状态：`APARunningstate >= 1` (搜索车位中及以后)
*   **代码位置**：`APAMap_ParkingOutTask()` 和 `APAMap_ParkingOutSetEndCarPosInOldCorSys()`。
*   **源码片段**：`if ((APAMap_GInputData.ParkReqPar.APAstate <= 3) && (APAMap_GInputData.ParkReqPar.APARunningstate >= 1))`
*   **作用逻辑**：只要系统进入了 `APA_RUNNING_STATE_SEARCHING_SLOT_E` (搜索车位) 且尚未完全接管执行器，就处于**“早期准备阶段”**。
    *   在任务调度口，这会触发 `APAMap_ParkingOutDebugInit()` 将各种复杂的入侵标志位（如 `bFsdInRightOfEndCarPosFlag`）初始化。
    *   在终点设定函数中，这会触发 `is_end_pos_initialized = TRUE`。由于车辆还没怎么动（更没越过锚点），系统允许**完全从零去计算默认的泊出终点坐标**。

### 3. 状态：`APARunningstate >= 4` (执行器连接中/车辆开始运动)
*   **代码位置**：主控函数 `APAMap_ParkingOutTask()`。
*   **源码片段**：`if((APAMap_GInputData.ParkReqPar.APARunningstate >= 4) && (APAMap_GInputData.ParkReqPar.Request_cmd == 1) && (request_cnt == lastreqcnt))`
*   **作用逻辑**：这是地图模块**最核心的分水岭**！当状态达到 `APA_RUNNING_STATE_ACTUATOR_CONTROL_CONNECTING_E` 甚至 `PARKING_FORWARD_FIRSTLY_E` 时，说明底层控制已经接管，**车辆已经开始动了**。
    *   此时，如果还是同一次请求（`request_cnt` 没变），代码会**直接进入轻量级的动态更新分支 `APAMap_ParkingOutUpDataMapInfo()`，随后立刻 `return`**。
    *   **架构意义**：一旦车动起来，绝对不能再重新建图（否则基础角点跳变会导致轨迹剧烈扭曲），只能拿最新的超声波/FSD雷达数据对现有边界进行“缝缝补补”（避障），保障行驶安全。

### 4. 状态：`APARunningstate >= 6` (第N步泊车中/驶出车位)
*   **代码位置**：阶段切换阀门函数 `APAMap_ParkingOutUpDataMapInfoBySlotCorInfo()`。
*   **源码片段**：`if ((APAMap_GInputData.ParkReqPar.APARunningstate >= 6) && ... && ((Request_cmd == 2) || (Request_cmd == 7))) { bAfterNewAnchorPointFlag = TRUE; }`
*   **作用逻辑**：当状态进入 `APA_RUNNING_STATE_PARKING_STEPN_E`，说明车辆已经深入执行泊出动作（比如已经揉过几把库，或者车头已经伸出车位）。
    *   此时配合指令，会置起极其关键的 **`bAfterNewAnchorPointFlag = TRUE`（锚点转换标志位）**。
    *   **架构意义**：这个标志位一旦置起，系统就会**永久冻结**车位内侧的角点（Obj1/Obj2）不再更新，将防碰撞的注意力全部转移到车身外侧的外部通道上。防止车尾开出时，雷达扫到路牙子把它误认作车位角点。

### 5. 状态：`APARunningstate >= 7` (到达终点或失败)
*   **代码位置**：主控函数 `APAMap_ParkingOutTask()` 的尾部。
*   **源码片段**：`if (APAMap_GInputData.ParkReqPar.APARunningstate >= 7) { APAMap_ParkingOutDebugInit(); }`
*   **作用逻辑**：当状态到达 `APA_RUNNING_STATE_PARKING_END_POSITION_ACHIEVED_E` (到达终点) 或 `APA_RUNNING_STATE_PARKING_FAIL_E` (泊车失败) 时，宣告单次泊出任务彻底结束。
    *   系统会再次调用 `APAMap_ParkingOutDebugInit()` 强行清理和重置所有的地图防撞标志位、宽窄通道标志位等。
    *   **架构意义**：这是一种“优雅的谢幕”。防止上一次泊出成功的终点残留数据（或者是失败时畸形的碰撞边界），在下一次功能唤醒时导致图污染，确保系统状态整洁。

**总结：**
这套 `APARunningstate` 状态机制，完美地指导了地图构建代码的生命周期。0 是清理，1-3 是静态初始化画线，4-5 是动态修补防撞，6 是锁死车位关注外部，7-8 是谢幕重置。

## Map相关数据
APAMap_GInfo
APAMap_GInputData
First APAMapParkout Build cmd(6) :建立坐标系
First APAMapParkout Build cmd(1) ：建立坐标系完成开始泊车
log 中 obj 定主要看
```
==PAOffset(0)(-1500.000000,2030.335693),Max(2406.335693),PreOffsetY(-1500.000000,-1000.000000),FrontMidSnsDis(742),RearMidSnsDis(376)))
==PAOffset(1)(-1500.000000,781.107178),Max(1523.107178),PreOffsetY(-1500.000000,-1000.000000),FrontMidSnsDis(742),RearMidSnsDis(376)))
```
如果 PAOffset中的坐标值为+ 则说明被压缩 若为负值则没有被压缩 上述说明Obj1 y被压缩到2030mm Obj2 y被压缩 781mm

```
=BSegData== 打印的是原来的点
=NSegData== 打印的是新的点 只看后面的值 为新增的点数和坐标
```

```
# SUPPORT_PARKING_OUT_SYSTEM_TEST 仿真宏


APUMAP_SOMEIP_MAJOR_VERSION  改为0x01
```

对向cal：
```
MaxDefaultRoadWith
```


```
==Carry out 看车位构造： 1 虚拟构造  2 由真实车位框构造
```

```
AlgCom_CheckIfGivenPtInthePolygonRegion 判断是否车是否在车位框内
```


```
bUpdataCalBoundaryFlag 车辆是否在车位内 True为在车位内
APAMap_ParkingOutCalBoundaryByParkOutInfo 锚点转换后重置主边界，转换前两个边界都会重置
```


### 左右边界定义
 `ParkSide == APA_CAR_PARK_AT_RIGHT_SIDE` 在水平泊出时竟然被赋值给了 `LeftBoundary`（逻辑完全反相），并且距离阈值也截然不同
水平泊出（侧方停车）的赋值逻辑之所以和垂直/斜列泊出“格格不入”，是因为底层隐藏着两个核心的物理与几何学原因：
为什么左右边界的赋值完全“反相”？
这是由于**车身停放姿态相差了 90 度**，导致局部标系下的“左右拓扑关系”发生了空间折叠。
用第一人称视角（司机坐在车里）推演一下：

**🚗 场景 A：垂直泊出（倒车入库）**

* **假设**：你的车位在马路的**左侧 (`LEFT_SIDE`)**。
* **姿态**：你停在车位里，车头垂直正对着马路。
* **感知识别**：此时，你左手边停着一辆车，右手边停着一辆车。车位的左边界就在你的**左侧**。
* **代码映射**：非常符合人类直觉。`ParkSide == LEFT` $\rightarrow$ `MainSlotBord = LeftBoundary`。

**🚙 场景 B：水平泊出（侧方停车）**

* **假设**：你的车位在马路的**右侧 (`RIGHT_SIDE`)**。
* **姿态**：你停在车位里，车头**平行**于马路（顺着车道线方向）。
* **感知识别**：此时，你的右手边是马路牙子（Curb），而**开放的马路和你的车位出口，其实在你的左手边**！前车 (Obj1) 和后车 (Obj2) 的防撞角点，也是分布在车身的**左前方和左后方**。
* **代码映射**：因为出口和障碍物都在左边，雷达和视觉传感器扫描到的车位边界数据，都被存进了局部坐标系的 `LeftBoundary` 数组中。
* **结论**：这就是为什么代码里会出现反直觉的 `ParkSide == RIGHT` $\rightarrow$ `MainSlotBord = LeftBoundary`！


# 流程
建立坐标系成功后 坐标原点为(0, 0)


# endpos
对于纯画线车位（代码中称为视觉车位 VPLSlot），算法在设定 `Obj1`、`Obj2` 角点以及各泊出模式下的目标落客点 `EndPos` 时，有一套非常严格的几何提取规则与常量设定。

以下是具体的逻辑解析：

### 1. 纯画线车位（VPL车位）的 `Obj1` 和 `Obj2` 设定
针对纯视觉识别的画线车位，代码主要通过 `APAMap_ParkingOutGetSlotInfoFromVPLSlotPts` 函数从底层的四个车位角点（`pVPLSlotPts[0~3]`）中提取出 `Obj1` 和 `Obj2`。

算法会根据**泊出模式（ParkOutMode）**和**车位在车辆的左侧还是右侧（bSlotDataAtRigthSide）**，通过下标映射（Index）直接从四边形的角点中强行锁定开口处的两个角点：

*   **水平泊出 (Parallel)**：
    *   **右侧车位**：`Obj2` 是左上角 (`0`)，`Obj1` 是左下角 (`1`)。
    *   **左侧车位**：`Obj2` 是右上角 (`3`)，`Obj1` 是右下角 (`2`)。
*   **车头泊出 (包含直进直出和转向，Head Go Straight / Turn Round)**：
    *   **右侧车位**：`Obj2` 是右上角 (`3`)，`Obj1` 是左上角 (`0`)。
    *   **左侧车位**：`Obj2` 是左上角 (`0`)，`Obj1` 是右上角 (`3`)。
*   **车尾泊出 (包含直进直出和转向，Rear Go Straight / Turn Round)**：
    *   **右侧车位**：`Obj2` 是左下角 (`1`)，`Obj1` 是右下角 (`2`)。
    *   **左侧车位**：`Obj2` 是右下角 (`2`)，`Obj1` 是左下角 (`1`)。

在确定了下标后，代码直接将角点坐标赋值：`*pObj2Pt = pVPLSlotPts[Obj2PtIndex]; *pObj1Pt = pVPLSlotPts[Obj1PtIndex];`。同时，以这两个开口角点的连线及其与内部角点的连线作为基准，计算出车位的角度 `Obj2Ang` 和 `Obj1Ang`。

---

### 2. 不同泊出模式下的 `EndPos`（目标坐标与车身角度）如何设定
落客点 `EndPos` 的计算由核心函数 `APAMap_ParkingOutSetEndCarPosInOldCorSys` 负责。

需要特别注意的是，`EndPos` 的基础坐标是在**锚点坐标系（以 `Obj2` 为原点，车位开口线为轴）**下计算的。在该局部坐标系中，$Y$ 轴代表沿着车位开口向前的通道方向，负 $X$ 轴代表垂直于车位开口向外（马路中央）的方向。

在没有障碍物侵占（`UpdateCntOffsetX/Y = 0`）的理想情况下，代码设定了以下明确的相对坐标常量：

**坐标位置 ($X, Y$) 设定：**
*   **水平泊出 (Parallel)**：
    *   **X坐标**：`-(半车宽 + 950mm)`。即车辆驶出车位后，车身中心位于车位线外侧距离半车宽外加 0.95 米的安全通道上。
    *   **Y坐标**：`2000mm`。即车辆向前行驶 2 米。
*   **斜列泊出 (Angled / Ladder)**：
    *   **X坐标**：如果识别为阶梯斜列（Ladder），X向外停靠得更远为 `-(半车宽 + 2000mm)`；如果是普通斜列，X 为 `-(半车宽 + 1100mm)`。
    *   **Y坐标**：`5000mm`。斜列车位需要更大的纵向安全驶出距离，设定为向前 5 米。
*   **垂直泊出 (Perpendicular)**：
    *   **X坐标**：`-(半车宽 + 3100mm)`。由于垂直泊出需要整个车身完全倒出或开出车位，所以横向驶出距离极大（3.1 米加上半车宽）。
    *   **Y坐标**：`4500mm`。纵向推进 4.5 米。

*(注：如果 FSD 或超声波检测到终点位置有障碍物侵占，代码会利用 `UpdateCntOffsetX` 等变量，以 100mm 或 150mm 为步长，向反方向进行网格化偏移，直到找到安全点)*。

**车身角度 (CarAng) 设定：**
*   **基础角度**：所有模式下，落客点的初始车头方向 `EndPosCarAng` 默认继承自车位角度 `Obj2Ang`。
*   **车尾直出反转**：如果当前是 **车尾直出模式 (Rear Go Straight)**，因为车是倒着出来的，为了保证落客时车头朝向正确，代码会对角度增加 180 度进行翻转（`EndPosCarAng += M_PI`）。
*   **动态矫正 (通道对齐)**：这只是初始地图构建时的静态计算。当车辆起步后，如果系统成功融合了外部的**车道线 (LaneLine)** 或者**车位参考线 (RefercLine)**，此时代码会将 `EndPos.CarAng` 强制刷新，使最终落客时的车身与外部真实车道线保持绝对平行。

# 功能函数
## APAMap_ParkingOutCalSlotInfo()
实现车位框构建、电子围栏构建、Obj1和Obj2更新、终点位置更新。
### APAMap_ParkingOutCalSlotBorderPtByParkOutSlotInfo　
获取有效车位框、进一步构造电子围栏，再确定锚点和Obj1、Obj2
#### AlgCom_GetParkOutEightMode （Map.cpp)
确定八个泊出方向
赋值到 `ParkOutEightMode`

#### APAMap_ParkingOutCalSlotParByVPLSlotInfoFromTotalMap
变量
```
Data[4]：车位框点 
```


##### APAMap_ParkingOutBuildCurCarPosSlotByTwoNearbySlot
斜列车位或者阶梯车位 根据相邻两侧车位框来构建当前车位框
```
NearBySlotNumsByLadder:当前车位周围的阶梯车位数量
NearBySlotNumsByAngled：周围的斜列车位数量
NearBySlotNumsByPerpen：周围垂直车位的数量

```

```
变量：
pCurSegData：输出参数，存放推算出来的自车车位 4 个角点坐标。
pFirstSegData / pSecondSegData：输入参数，第一个和第二个相邻车位（比如左侧和右侧车位）的原始 4 个角点坐标。
FirstNearByCarPosSlot / SecondNearByCarPosSlot：这两个相邻车位的参考坐标点（锚点）。
Data1Index / Data2Index：指示这两个相邻车位与自车的相对空间拓扑关系（如左侧、右侧、上方位、下方位）。
Label：需要构造的目标车位类型（如 Obj_Label_Angled_Slot 斜列车位、Obj_Label_Ladder_Slot 阶梯车位、Obj_Label_Perpen_Slot 垂直车位等）。
```
利用相邻车位 构造自车位
![](../../../images/assets/open_space_roi_decider.assets/Pasted%20image%2020260403173940.png)


##### APAMap_ParkingOutBuildCurCarPosSlotByOneSideNearbySlot
利用 单边有效车位构造 自车位
一些场景或者情况判断
###### APAMap_ParkingOutBuildSlotByOneSideNearbySlot
主要构建车位的函数  (需要对各种if 分支画图一下)


### APAMap_ParkingOutCalSlotBorderPtByParkOutInfo
在无 有效车位框时构造虚拟车位框，并确定锚点Obj1 2

### APAMap_ParkingOutCalSlotBorderPtByFSDAndODMapInfo
用Sensor、FSD 和OD 更新Obj



# 可以优化的地方
1.APAMap_ParkingOutGetSlotInfoFromVPLSlotPts 中点顺序修改 2处相同的代码
![](../../../images/assets/open_space_roi_decider.assets/Pasted%20image%2020260409162032.png)

2.endpos 更新优化一下
WI-10062 - 合并分支-EC24厦门-完全泊出-27.05131520 左方柱角点右车 极小车位2.46米 对向车5.4米 8步泊入 后视镜越过角点方柱才折叠 左前泊出第1趟对向无边界，第2趟对向态车经过泊车退出 NG
WI-10058 

 边界变化endpos一直变
