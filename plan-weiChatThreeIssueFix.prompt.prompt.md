## Plan: WeiChat 三问题联合修复

按你的确认，我建议用“先修正确性、再修体验”的顺序推进：先把会话隔离链路修好（解决点谁都同一对话框），再把消息可读性修好（解决自己消息看不清、别人消息发虚），最后把侧边栏全量改成可点击并补功能反馈。

**Steps**
1. Phase 1 会话隔离与消息归属  
在接收消息流程增加 currentPeer 过滤；引入按好友维度的消息缓存；切换好友时重建对应会话视图，不再复用同一消息列表。
2. Phase 1.1 刷新联动稳定  
修复好友列表定时刷新导致 currentPeer 错位或被清空的问题，刷新后保持当前会话选中并恢复其历史消息。  
依赖：Step 1
3. Phase 2 消息可读性修复  
调整消息气泡文本与背景对比度，降低过度透明，统一消息字体家族/字号/字重，优先解决“自己消息不可见”和“他人消息模糊”。
4. Phase 2.1 渲染清晰度增强  
统一高 DPI 与抗锯齿策略，确保 Windows 100%/125% 缩放下消息文本边缘清晰。  
可与 Step 3 并行
5. Phase 3 侧边栏可点击改造  
把侧边栏与功能栏纯展示图片控件改成可点击按钮并接入你确认的映射：聊天/通讯录/发现/我 + 设置/通话/更多。  
依赖：Step 1
6. Phase 3.1 占位反馈  
未落地功能统一加“功能开发中”反馈，保证每个图标点击都有响应。  
依赖：Step 5
7. Phase 4 编译与回归  
编译客户端和服务端，执行多账号收发、切换好友、主题切换、侧边栏全点击回归；若 exe 被占用导致链接失败，先停止进程后重试。  
依赖：Step 1-6

**Relevant files**
- [EasyChatProject/EasyChat-main/WeiChat/chat_dialog.h](EasyChatProject/EasyChat-main/WeiChat/chat_dialog.h) - 会话缓存结构、侧边栏按钮成员和槽声明
- [EasyChatProject/EasyChat-main/WeiChat/chat_dialog.cpp](EasyChatProject/EasyChat-main/WeiChat/chat_dialog.cpp) - 消息接收过滤、会话切换重建、好友刷新保持选中、侧边栏信号槽绑定
- [EasyChatProject/EasyChat-main/WeiChat/chat_dialog.ui](EasyChatProject/EasyChat-main/WeiChat/chat_dialog.ui) - 侧边栏图标控件识别与替换入口
- [EasyChatProject/EasyChat-main/WeiChat/chatbubble.h](EasyChatProject/EasyChat-main/WeiChat/chatbubble.h) - 消息文本渲染参数入口
- [EasyChatProject/EasyChat-main/WeiChat/chatbubble.cpp](EasyChatProject/EasyChat-main/WeiChat/chatbubble.cpp) - 气泡字体/渲染/布局细化
- [EasyChatProject/EasyChat-main/WeiChat/style/stylesheet.qss](EasyChatProject/EasyChat-main/WeiChat/style/stylesheet.qss) - 消息气泡与主题对比度主样式
- [EasyChatProject/EasyChat-main/WeiChat/main.cpp](EasyChatProject/EasyChat-main/WeiChat/main.cpp) - 高 DPI 与全局字体渲染策略

**Verification**
1. A/B 双好友互发时，当前窗口只显示 currentPeer 消息，切换后展示对应历史，不再串聊。
2. 深浅主题下发送和接收消息都清晰可读，重点验证“自己消息可见性”和“他人消息锐利度”。
3. 开启好友定时刷新后，当前会话不跳变、不混乱。
4. 侧边栏与功能栏全部图标可点击且有反馈；未实现功能出现占位提示。
5. 使用 Qt 5.14.2 mingw73_32 完成客户端与服务端编译，无新增错误。

**已确认决策**
1. 会话切换显示该好友历史消息。
2. 侧边栏按常见微信映射实现。
3. 暂未完成功能先提供“开发中”提示，先保证可点击可反馈。

计划已同步到会话文件：/memories/session/plan.md。
如果你确认，我就保持这个方案不再变动，交给实现阶段直接按步骤落地。
