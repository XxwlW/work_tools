在 DecideIfEableBackwardSearchPreferMoveStraight 函数中，这四个变量是用于控制逆向搜索（backward search）阶段车辆行为的策略参数，其核心目标是：在靠近目标点时，鼓励车辆优先保持直线行驶，避免过早或不必要的急转弯，从而提高泊车规划的安全性和成功率。

它们的具体作用如下：

1. backward_search_least_dis_befor_rspath_
作用: 定义了开始启用“优先直行”策略的最小距离阈值。
详细解释:
当车辆在逆向搜索过程中，从起点到当前节点的累计轨迹长度（GetTrajDisToStartNode()）小于或等于这个值时，系统才会激活后续的“优先直行”惩罚机制。
这相当于一个“安全区”或“准备区”。只有当车辆进入这个区域后，才需要考虑“走直线”的问题。如果距离太远，车辆可以自由地进行大角度转向来调整姿态，无需受限。
示例: 如果设置为 veh_length * 0.65，意味着当车辆离终点的距离小于其自身长度的 65% 时，才开始执行直行优先策略。
2. backward_search_prefer_move_straight_dis_
作用: 定义了**“优先直行”策略生效的范围**。
详细解释:
这个值通常大于 backward_search_least_dis_befor_rspath_。
它设定了一个更大的距离区间 [0, backward_search_prefer_move_straight_dis_]。在这个区间内，系统会根据 backward_search_prefer_move_straight_enhance_first_factor_ 和 backward_search_prefer_move_straight_enhance_second_factor_ 对方向盘转角施加额外惩罚。
其目的是让车辆在接近终点的整个关键区域内，都倾向于走直线。
示例: 设置为 veh_length * 0.80，表示在距离终点 80% 车长范围内，都要优先考虑直行。
3. backward_search_prefer_move_straight_enhance_first_factor_
作用: 增强第一阶段的惩罚系数。
详细解释:
这个变量用于在 backward_search_least_dis_befor_rspath_ 到 backward_search_prefer_move_straight_dis_ 这个较近的区间内，对方向盘转角的惩罚进行放大。
在这个区域，车辆应该最严格地保持直线，因此惩罚因子被设得更高（例如 5.0 或 1.5），以强烈抑制任何大的转向动作。
它与 backward_search_prefer_move_straight_enhance_second_factor_ 一起，构成了分段式惩罚策略。
示例: 值为 5.0 意味着，如果车辆在此区域内转向过大，其代价将被放大 5 倍。
4. backward_search_prefer_move_straight_enhance_second_factor_
作用: 增强第二阶段的惩罚系数。
详细解释./images/ 0 到 backward_search_least_dis_befor_rspath_ 这个更远的区域内，对方向盘转角的惩罚进行放大。
相比于第一阶段，这个惩罚的强度通常较低，但仍然存在，以确保车辆在更远的距离上就有一个“尽量走直线”的倾向，而不是等到最后一刻才突然改变方向。
它是一个渐进式的引导，帮助车辆提前规划出一条平滑的、最终能对准车位的路径。
示例: 值为 3.5 或 1.0，表示在稍远一点的区域，转向惩罚也会被放大，但幅度小于第一阶段。
