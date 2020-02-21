## Type Coach
Type coach 是一个基于 ncurses 库的命令行程序，用于练习仓颉输入法。

### 程序结构
``` shell
├── articles                          # 存放练习文章
│   ├── sample.txt
├── dicts                             # 码表
│   ├── cangjie6.dict.yaml
│   └── cangjie6.extsimp.dict.yaml
├── README.md
├── trainner
├── trainner.c
├── utils.c                           # 宽字符串处理及哈希表
├── utils.h
├── utils.o
├── windows.c                         # ncurses 窗口
├── windows.h
└── windows.o
```

### 开发进度
- DONE 文章选单
- TODO 码表选单（暂缓）
- TODO 上下控制滾屏（暂缓）
- DONE 删除文字时向上滚屏
- DONE 打字进度条
- DONE 完善彩色模式
- TODO 键位练习模式
- TODO 可关闭提示
- DONE 词组提示
- DONE 提示窗口自适应位置
- TODO 数据统计
- DONE 自动跳过文章中的空行
- TODO 仓颉单字编码和词组编码不同，单独处理两组编码

### 致谢
#### 蒼頡檢字法
感谢朱邦復先生。

#### 蒼頡檢字法單字碼表
單字碼表自百度貼吧「倉頡」吧網友风易辞得，又： 蒼頡六代構詞碼碼表。

- 由雪齋、惜緣整理於 2013-01-29
- crazy4u根据雪齋發佈的碼表增二萬餘字 2013-02-19
- crazy4u增易經八卦符號 2013-03-26
- 單字編碼兼顧宋體、細明體字形 crazy4u 2013-04-02
- 又增一千字 crazy4u 2013-04-27
- 增加組詞分隔符、正體字字頻 雪齋 2013-05-30
- 採用相對字頻 雪齋 2013-12-09
- 增加擴展E區字 雪齋 2014-11-03
- 增加擴展F區字 Fszhouzz 2017-12-09

License: GPL

#### 蒼頡檢字法簡體詞典
蒼頡檢字法簡體詞典，來自[搜狗互聯網詞庫](http://www.sogou.com/labs/dl/w.html)

- 雪齋整理

License: GPL
