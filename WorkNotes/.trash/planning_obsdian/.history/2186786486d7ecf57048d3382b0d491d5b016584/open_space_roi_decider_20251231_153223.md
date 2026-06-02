###　bool OpenSpaceRoiDecider::GetParkingSpot

获取车位四个顶点



## bool OpenSpaceRoiDecider::SetParkingSpotEndPose



为 Open Space 规划器生成一个“驶离停车位后短暂跟线行驶”的局部目标位姿（含位置、朝向、速度），作为非结构化轨迹优化的终点约束

Vec2d::SelfRotate 坐标系旋转函数

![image-20251230130413093](../images/open_space_roi_decider.assets/image-20251230130413093.png)
