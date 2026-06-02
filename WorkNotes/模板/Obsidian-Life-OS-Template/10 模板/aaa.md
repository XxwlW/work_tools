<%*
const dv = app.plugins.plugins["dataview"].api;
const pages = dv.pages("#想法")
for (let page of pages) {
  tR += `- ${page.file.name}\n`;
}
tR += tp.web.daily_quote().then(response => response.json())
    .then(data => {
        console.log("拿到数据：", data);
    })
    .catch(error => {
        console.error("出错了", error);
    });
%>
- 语音输入与输入法引擎的联动-3640
- 拼音加辅码的辅-3951
- 关于更好地组织笔记-3806
- 六子吃粉-0917
- ya音部首选重-3214
- ab部首筛选-1043
- obsidian
- 2025-04-17 周四
- 2025-04-13 周日
- 2025-04-06 周日
[object Promise]



