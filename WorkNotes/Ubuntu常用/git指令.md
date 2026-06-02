ly对比不同分支内容11月工作记录

Planning基础结构及调用

git指令

git difftool --tool=bc dev_LocTrajPlannGen2.0  --dir-diff ./src/





git merge时 要注意proto 解冲突时 插件解的冲突可能保留不完全 要确认一下





在2818 3146 ./git/hooks 下加入了钩子 

```
hook-config.sh
logs
post-checkout.sh
```

hooks 在执行git 指令后会执行 hooks中的脚本

post-checkout：在2818下 切换分支时 执行编译脚本 并 恢复丢失代码

 3146下 当out文件夹中文件时 才进行tte_make_clean.sh脚本进行清空





补丁功能跨工作树应用:

(cd ../dev_Loc && git format-patch -1 HEAD --stdout) | git am -3

```
(cd ../dev_Loc && ...)：在子shell中切换到 ../dev_Loc 目录

git format-patch：生成格式化补丁文件

-1：只处理最近的1个提交

HEAD：从当前提交开始

--stdout：将补丁输出到标准输出，而不是文件
```

| git am -3

```
应用补丁 
| : 管道符 将前一命令作为下一个命令的输入
git am: 应用邮箱格式的补丁 (git am --abort git am --continue)
-3：使用三方合并策略，遇到冲突时会暂停
```





ubuntu git  版本回滚

```
sudo rm -f /usr/bin/git
sudo mv /usr/bin/git-2.17.1 /usr/bin/git
git --version
```



```
# 查看两个分支的差异统计
git diff branch1..branch2 --stat

# 查看两个分支有哪些不同的提交
git log --left-right --graph --oneline branch1...branch2
```



```
# 标记哪些提交是 cherry-pick 的（= 重复）
git log --oneline --cherry-mark --graph
```


查看提交记录
```
for branch in $(git branch -a --list 'origin/*' 'refs/heads/*' | grep -v HEAD | sed 's|  ||'); do   count=$(git log "$branch" --author="xj.wang" --after="2026-04-01" --before="2026-04-30" --oneline | wc -l);   [ "$count" -eq 0 ] && continue;   echo "📁 $branch ($count 条)";   git log "$branch" --author="xj.wang" --after="2026-04-01" --before="2026-04-30" --pretty=format:"  %h | %ad | %s" --date=short;   echo -e "\n"; done

```