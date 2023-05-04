# 一、概述
基于cppCo协程库的RPC框架

# 二、性能测试
2.1 HTTP echo 测试 QPS
环境配置：windows下的wsl，CPU为**12核**

测试工具：**wrk**: https://github.com/wg/wrk.git

部署信息：wrk 与 corpc 服务部署在都在同一个wsl中, 关闭日志输出

测试命令与结果如下：
```
// -c 为并发连接数，按照表格数据依次修改
wrk -c 1000 -t 8 -d 30 --latency 'http://127.0.0.1:19999/qps?id=1'
```


#TODO:
- 时间轮算法 √
- pb及rpc实现 √
- 应用libco中的上下文切换汇编代码 √
- 实现异步的RPC调用
- 实现m:n协程，采用共享协程池的模式
