# RuntimeSample


## 文件列表以及描述

### debugableruntime

可编译的 runtime 版本

### Hash Map

剥离出 runtime 中的 hash map。

KSNXHashTable.c KSNXHashTable.h KSNXMapTable.c KSNXMapTable.h 这四个文件是从 runtime 中剥离出来的 maptable 和 hashtable，之所以加上前缀 “KS” ，是为了防止和 runtime 中的冲突。

### LogLog

剥离出 runtime 中的日志系统。使用方式很简单，直接跑 demo 工程即可。




