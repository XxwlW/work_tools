
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

# 流程
建立坐标系成功后 坐标原点为(0, 0)


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
![](../../../assets/Pasted%20image%2020260403173940.png)


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
![](../../../assets/Pasted%20image%2020260409162032.png)

2.endpos 更新优化一下
WI-10062 - 合并分支-EC24厦门-完全泊出-27.05131520 左方柱角点右车 极小车位2.46米 对向车5.4米 8步泊入 后视镜越过角点方柱才折叠 左前泊出第1趟对向无边界，第2趟对向态车经过泊车退出 NG
WI-10058 

 边界变化endpos一直变
