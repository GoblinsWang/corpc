# CORPC 架构设计文档

## 一、项目概述

CORPC 是一个基于协程的高性能异步 RPC 框架，使用 C++11 开发。核心特点：

- **协程调度**：用户态协程，支持创建、切换、挂起、恢复
- **双协议支持**：HTTP/1.1 + Protobuf 自定义二进制协议
- **高性能网络**：基于 epoll 的非阻塞 I/O 多路复用
- **RPC 服务**：完整的服务注册、方法调用、客户端-服务器通信
- **性能**：12核 WSL 环境下 HTTP echo 测试 QPS 达到 19万+

---

## 二、整体架构

### 2.1 分层架构图

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Application Layer                           │
│                    (HTTP Servlet / PB Service)                      │
└─────────────────────────────────────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        TcpServer / TcpClient                        │
│              (连接管理、协议选择、时间轮、Dispatcher)                 │
└─────────────────────────────────────────────────────────────────────┘
                                 │
              ┌──────────────────┼──────────────────┐
              ▼                  ▼                  ▼
        ┌──────────┐       ┌──────────┐       ┌──────────┐
        │ HTTP协议  │       │  PB协议   │       │  Codec   │
        │Dispatcher │       │Dispatcher │       │ (编解码)  │
        └──────────┘       └──────────┘       └──────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────┐
│                          Scheduler (单例)                           │
│                     (全局调度器，管理所有 Processor)                 │
└─────────────────────────────────────────────────────────────────────┘
                                 │
              ┌──────────────────┼──────────────────┐
              ▼                  ▼                  ▼
        ┌───────────┐      ┌───────────┐      ┌───────────┐
        │ Processor │      │ Processor │      │ Processor │
        │ (Thread 0)│      │ (Thread 1)│      │ (Thread N)│
        └───────────┘      └───────────┘      └───────────┘
              │                  │                  │
        ┌─────┴─────┐      ┌─────┴─────┐      ┌─────┴─────┐
        ▼           ▼      ▼           ▼      ▼           ▼
    ┌───────┐  ┌───────┐  ┌───────┐  ┌───────┐  ┌───────┐
    │Epoller│  │ Timer │  │Epoller│  │ Timer │  │Epoller│
    └───────┘  └───────┘  └───────┘  └───────┘  └───────┘
        │
        ▼
    ┌───────────────────────────────────────┐
    │         Coroutine Pool                │
    │  (每个连接一个协程处理 IO + 业务)       │
    └───────────────────────────────────────┘
```

### 2.2 目录结构

```
corpc/
├── corpc/                      # 核心源代码
│   ├── coroutine/              # 协程模块
│   │   ├── scheduler.cc/h      # 全局调度器
│   │   ├── processor.cc/h      # 协程处理器（每线程一个）
│   │   ├── coroutine.cc/h      # 协程类定义
│   │   ├── context.cc/h        # 协程上下文
│   │   ├── coctx_swap.S        # 汇编上下文切换（来自 libco）
│   │   ├── co_api.cc/h         # 协程 API 接口
│   │   ├── epoller.cc/h        # epoll 封装
│   │   ├── timer.cc/h          # 定时器
│   │   ├── fd_event.cc/h       # 文件描述符事件
│   │   ├── objpool.h           # 对象池
│   │   ├── mempool.h           # 内存池
│   │   ├── lock_free_ring_buffer.h  # 无锁环形缓冲区
│   │   ├── spinlock.h          # 自旋锁
│   │   └── rw_mutex.cc/h       # 读写锁
│   ├── log/                    # 日志模块
│   │   ├── logger.cpp/h        # 日志器
│   │   ├── file.cpp/h          # 文件管理
│   │   └── log.conf            # 日志配置
│   └── net/                    # 网络模块
│       ├── abstract_codec.h    # 编解码抽象基类
│       ├── abstract_data.h     # 数据抽象基类
│       ├── abstract_dispatcher.h  # 分发器抽象基类
│       ├── net_address.cc/h    # 网络地址封装
│       ├── net_socket.cc/h     # Socket 封装
│       ├── comm/               # 通用工具
│       ├── http/               # HTTP 协议实现
│       ├── pb/                 # Protobuf RPC 实现
│       └── tcp/                # TCP 连接管理
├── example/                    # 示例代码
├── docs/                       # 文档目录
└── README.md                   # 项目说明
```

---

## 三、协程系统设计

### 3.1 核心组件

| 组件 | 文件 | 职责 |
|------|------|------|
| **Scheduler** | scheduler.h/cc | 全局单例，管理所有 Processor，提供选择策略 |
| **Processor** | processor.h/cc | 独立线程 + 事件循环，管理本线程内的所有协程 |
| **Coroutine** | coroutine.h/cc | 协程实体，封装执行函数，状态管理 |
| **Context** | context.h/cc | 协程上下文，保存寄存器状态，管理栈内存 |
| **coctx_swap** | coctx_swap.S | 汇编实现，真正的上下文切换 |

### 3.2 协程上下文切换

采用腾讯 libco 的汇编代码实现用户态上下文切换：

```asm
# coctx_swap.S
# void coctx_swap(coctx* cur_ctx, coctx* target_ctx)
# 保存当前寄存器到 cur_ctx，恢复 target_ctx 的寄存器

coctx_swap:
    leaq (%rsp),%rax
    movq %rax, 104(%rdi)   # 保存 RSP
    movq %rbx, 96(%rdi)    # 保存 RBX
    ...                    # 保存其他寄存器

    movq 48(%rsi), %rbp    # 恢复 RBP
    movq 104(%rsi), %rsp   # 恢复 RSP
    ...                    # 恢复其他寄存器
    ret                    # 跳转到目标协程
```

寄存器布局定义：

```cpp
enum {
    kRBP = 6,      // 栈底指针
    kRDI = 7,      // 第一参数
    kRSI = 8,      // 第二参数
    kRETAddr = 9,  // 返回地址（将赋值给 RIP）
    kRSP = 13,     // 栈顶指针
};
```

### 3.3 协程生命周期

```
┌──────────┐     resume()     ┌──────────┐     yield()      ┌──────────┐
│ CO_READY │ ───────────────► │CO_RUNNING│ ───────────────► │CO_WAITING│
└──────────┘                  └──────────┘                  └──────────┘
                                   │                             │
                                   │ 协程执行完毕                  │ resume()
                                   ▼                             ▼
                              ┌──────────┐                  ┌──────────┐
                              │ CO_DEAD  │                  │CO_RUNNING│
                              └──────────┘                  └──────────┘
```

### 3.4 协程 API

```cpp
namespace corpc {

// 启动一个新协程
// tid: -1 表示自动选择 Processor，否则指定 Processor ID
// stackSize: 协程栈大小，默认 128KB
void co_go(std::function<void()> &func, int tid = -1, size_t stackSize = 128*1024);

// 当前协程休眠指定毫秒
void co_sleep(int64_t interval);

// 等待调度器结束
void sche_join();

}
```

---

## 四、Processor 设计

### 4.1 Processor 概念

**Processor = 线程 + 事件循环 + 协程调度器 + 协程池**

每个 Processor 是一个独立的执行单元，包含：

```
┌─────────────────────────────────────────────────────────────────┐
│                         Processor                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   ┌─────────────┐    ┌─────────────┐    ┌─────────────┐        │
│   │  Epoller    │    │   Timer     │    │  ObjPool    │        │
│   │  (epoll)    │    │  (timerfd)  │    │ (协程对象池) │        │
│   └──────┬──────┘    └──────┬──────┘    └─────────────┘        │
│          │                  │                                   │
│          ▼                  ▼                                   │
│   ┌─────────────────────────────────────────────────────┐      │
│   │                 Event Loop (事件循环)                │      │
│   │                                                     │      │
│   │   while (running) {                                 │      │
│   │       epoll_wait();           // 等待事件            │      │
│   │       执行 pending_tasks;      // 新协程首次执行     │      │
│   │       执行 active_tasks;       // IO就绪的协程恢复   │      │
│   │       清理 dead_coroutines;    // 回收完成的协程     │      │
│   │   }                                                │      │
│   └─────────────────────────────────────────────────────┘      │
│                                                                 │
│   ┌─────────────────────────────────────────────────────┐      │
│   │               Coroutine Management                   │      │
│   │                                                     │      │
│   │   m_pending_tasks  ──► 待执行队列 (无锁环形缓冲区)   │      │
│   │   m_active_tasks   ──► 就绪队列 (IO事件触发)         │      │
│   │   m_cors_set       ──► 所有存活协程集合              │      │
│   │   m_cur_coroutine  ──► 当前正在执行的协程            │      │
│   │   m_main_ctx       ──► 主上下文 (用于协程切换)       │      │
│   └─────────────────────────────────────────────────────┘      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 4.2 Processor 核心职责

| 职责 | 方法 | 说明 |
|------|------|------|
| **协程调度** | `resume()` / `yield()` | 决定执行哪个协程，何时切换 |
| **IO 事件监听** | `waitEvent()` | 通过 epoll 监听文件描述符事件 |
| **定时管理** | `wait()` | 管理定时器和超时事件 |
| **协程生命周期** | `getNewCoroutine()` / `killCurCo()` | 创建、回收协程 |

### 4.3 事件循环

```cpp
bool Processor::loop() {
    m_epoller = std::make_shared<Epoller>(this);
    m_timer = std::make_shared<Timer>(this);

    m_pLoop = new std::thread([this] {
        threadIdx = m_tid;
        m_status = PRO_RUNNING;

        while (PRO_RUNNING == m_status) {
            // 1. 清空上轮激活列表
            m_active_tasks.clear();

            // 2. epoll 等待事件
            m_epoller->getActiveTasks(timeout, m_active_tasks);

            // 3. 执行新加入的协程
            Coroutine *pNewCo = nullptr;
            while (m_pending_tasks.pop(pNewCo)) {
                m_cors_set.insert(pNewCo);
                resume(pNewCo);
            }

            // 4. 执行被 IO 唤醒的协程
            for (auto co : m_active_tasks) {
                resume(co);
            }

            // 5. 清理已完成的协程
            for (auto deadCo : m_removed_cors) {
                m_cors_set.erase(deadCo);
                m_cor_pool.delete_obj(deadCo);
            }
            m_removed_cors.clear();
        }
    });
    return true;
}
```

---

## 五、网络模型设计

### 5.1 Reactor + 协程模式

```
┌────────────────────────────────────────────────────────────────┐
│                        Processor Thread                         │
│                                                                │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                      Epoller (epoll)                      │  │
│  │   ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐    │  │
│  │   │ fd=100  │  │ fd=101  │  │ timerfd │  │ wakeupfd│    │  │
│  │   │ READ    │  │ WRITE   │  │ READ    │  │ READ    │    │  │
│  │   └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘    │  │
│  └───────│────────────│────────────│────────────│──────────┘  │
│          │            │            │            │              │
│          ▼            ▼            ▼            ▼              │
│  ┌────────────────────────────────────────────────────────┐   │
│  │              Coroutine for each Connection              │   │
│  │   ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐   │   │
│  │   │  Co_1   │  │  Co_2   │  │  Co_3   │  │  Co_N   │   │   │
│  │   │(fd=100) │  │(fd=101) │  │ (timer) │  │(wakeup) │   │   │
│  │   └─────────┘  └─────────┘  └─────────┘  └─────────┘   │   │
│  └────────────────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────────────────┘
```

### 5.2 连接处理流程

```cpp
// 1. 服务器启动 Accept 协程
void TcpServer::start() {
    m_acceptor.reset(new TcpAcceptor(m_addr));
    corpc::co_go(std::bind(&TcpServer::MainAcceptCorFunc, this), 0);
}

// 2. Accept 协程循环接受新连接
void TcpServer::MainAcceptCorFunc() {
    while (!m_is_stop_accept) {
        auto sock = m_acceptor->toAccept();
        TcpConnection::ptr conn = addClient(sock);
        conn->initServer();  // 为每个连接启动协程
    }
}

// 3. 每个连接一个协程处理 IO
void TcpConnection::MainServerLoopCorFunc() {
    while (!m_stop) {
        input();    // 读取数据（无数据时自动 yield）
        execute();  // 解码 + RPC 分发 + 业务处理
        output();   // 发送响应
    }
}
```

### 5.3 协程式 IO 等待

关键设计：**IO 操作自动挂起协程，事件到来时自动恢复**

```cpp
void Processor::waitEvent(FdEvent::ptr fd_event, int fd, int event) {
    fd_event->setCoroutine(m_cur_coroutine);  // 绑定当前协程
    m_epoller->addEvent(fd_event.get(), fd, event);
    yield();  // 挂起当前协程，切换回主上下文
    // ... 事件到来后，从这里恢复执行 ...
    m_epoller->delEvent(fd);
}
```

---

## 六、RPC 调用流程

### 6.1 PB 协议数据结构

```
┌──────────────────────────────────────────────────────────────────┐
│                          PbStruct                                │
├──────────────────────────────────────────────────────────────────┤
│ pk_len (4B)          │ 整个包长度                                 │
│ msg_req_len (4B)     │ 消息请求ID长度                              │
│ msg_req (变长)        │ 消息请求ID (用于匹配请求-响应)               │
│ service_name_len (4B) │ 服务名长度                                 │
│ service_full_name    │ 服务全名 (如 "QueryService.query_name")     │
│ err_code (4B)        │ 错误码                                     │
│ err_info_len (4B)    │ 错误信息长度                                │
│ err_info (变长)       │ 错误信息                                   │
│ pb_data (变长)        │ Protobuf 序列化的业务数据                   │
│ check_num (4B)       │ 校验和                                     │
└──────────────────────────────────────────────────────────────────┘
```

### 6.2 服务端 RPC 处理流程

```
Client Request
      │
      ▼
┌──────────────┐
│   input()    │ ──► 读取数据到 read_buffer
└──────────────┘
      │
      ▼
┌──────────────┐
│ PbCodec::    │
│  decode()    │ ──► 解码为 PbStruct
└──────────────┘
      │
      ▼
┌──────────────────────────────────────────────────────┐
│ PbRpcDispatcher::dispatch()                          │
│                                                      │
│  1. 解析 service_full_name → service_name + method   │
│  2. 从 m_service_map 查找 Service 对象               │
│  3. 获取 MethodDescriptor                            │
│  4. 创建 request/response 对象                       │
│  5. request->ParseFromString(pb_data)               │
│  6. service->CallMethod(method, ...)                │
│  7. response->SerializeToString(&reply.pb_data)     │
│  8. 编码响应到 write_buffer                          │
└──────────────────────────────────────────────────────┘
      │
      ▼
┌──────────────┐
│   output()   │ ──► 发送响应数据
└──────────────┘
```

### 6.3 使用示例

```cpp
// 1. 定义 Proto 服务
// test.proto
service QueryService {
  rpc query_name(QueryRequest) returns (QueryResponse);
}

// 2. 实现服务
class QueryServiceImpl : public QueryService {
public:
    void query_name(google::protobuf::RpcController* controller,
                    const QueryRequest* request,
                    QueryResponse* response,
                    google::protobuf::Closure* done) override {
        response->set_name("hello " + request->id());
        if (done) done->Run();
    }
};

// 3. 启动服务器
int main() {
    auto addr = std::make_shared<corpc::IPAddress>("127.0.0.1", 8080);
    corpc::TcpServer server(addr, corpc::Pb_Protocal);

    auto service = std::make_shared<QueryServiceImpl>();
    server.registerService(service);

    server.start();
    corpc::sche_join();
    return 0;
}
```

---

## 七、定时器与时间轮

### 7.1 定时器设计

使用 `timerfd` + `epoll` 实现高效定时：

```cpp
class Timer : public FdEvent {
    // 使用 multimap 按到达时间排序
    std::multimap<int64_t, TimerEvent::ptr> m_pending_events;

    void addTimerEvent(TimerEvent::ptr event);
    void onTimer();  // 定时触发时执行
};
```

### 7.2 时间轮核心原理

时间轮是一种高效的定时器管理数据结构，特别适合管理大量连接的超时检测。

```
      ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
      │ 槽0 │ 槽1 │ 槽2 │ 槽3 │ 槽4 │ 槽5 │ 槽6 │ 槽7 │ 槽8 │ 槽9 │
      │     │     │     │     │     │     │     │     │     │     │
      └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
         ▲
         │
       指针

   参数配置：
   - bucket_count = 10 (10个槽位)
   - interval = 6 (每6秒指针移动一次)
   - 总超时时间 = 10 × 6 = 60秒
```

### 7.3 时间轮数据结构

```cpp
// 时间轮主体
class TcpTimeWheel {
private:
    int m_bucket_count{10};    // 槽位数量
    int m_inteval{6};          // 每槽间隔(秒)
    int m_cur_bucket{0};       // 当前指针位置
    TimerEvent::ptr m_event;   // 定时器事件

    // 每个槽位存放一组连接槽
    std::vector<std::vector<TcpConnectionSlot::ptr>> m_wheel;
};

// 连接槽（核心设计）
template <class T>
class AbstractSlot {
public:
    AbstractSlot(weakPtr ptr, std::function<void(sharedPtr)> cb)
        : m_weak_ptr(ptr), m_cb(cb) {}

    ~AbstractSlot() {
        // ★ 析构时：如果连接还活着，执行超时回调
        sharedPtr ptr = m_weak_ptr.lock();
        if (ptr) {
            m_cb(ptr);  // 调用 shutdownConnection
        }
    }

private:
    weakPtr m_weak_ptr;                    // TcpConnection 的弱引用
    std::function<void(sharedPtr)> m_cb;   // 超时回调
};
```

### 7.4 时间轮工作流程

#### 初始化

```cpp
TcpTimeWheel::TcpTimeWheel(int bucket_count = 10, int interval = 6) {
    // 1. 创建空槽位
    for (int i = 0; i < bucket_count; ++i) {
        m_wheel.emplace_back(vector<TcpConnectionSlot::ptr>());
    }

    // 2. 注册定时器，每 6 秒触发 loopFunc
    m_event = make_shared<TimerEvent>(6000, true, loopFunc);
    scheduler->getProcessor(0)->getTimer()->addTimerEvent(m_event);
}
```

#### 刷新连接

```cpp
void TcpTimeWheel::fresh(TcpConnectionSlot::ptr slot) {
    // 放入"上一个"槽位，至少存活 bucket_count-1 个周期
    int index = (m_cur_bucket - 1 + m_bucket_count) % m_bucket_count;
    m_wheel[index].emplace_back(slot);
}
```

#### 定时触发

```cpp
void TcpTimeWheel::loopFunc() {
    // ★ 清空当前槽位，Slot 析构时触发超时回调
    m_wheel[m_cur_bucket].clear();
    m_cur_bucket = (m_cur_bucket + 1) % m_bucket_count;
}
```

### 7.5 超时检测时间线

```
t=0s (连接A建立，放入槽4)
  ┌─────┬─────┬─────┬─────┬─────┬─────┐
  │ 槽0 │ 槽1 │ 槽2 │ 槽3 │ 槽4 │ ... │
  │  ↑  │     │     │     │ {A} │     │
  │指针 │     │     │     │     │     │
  └─────┴─────┴─────┴─────┴─────┴─────┘

t=6s   指针移到槽1，清空槽0
t=12s  指针移到槽2，清空槽1
...

t=60s  指针回到槽0，清空槽4
        └──► Slot析构 → 触发超时回调 → shutdownConnection()
```

### 7.6 连接注册与刷新

```cpp
// 连接建立时注册
void TcpConnection::registerToTimeWheel() {
    auto cb = [](TcpConnection::ptr conn) {
        conn->shutdownConnection();  // 超时回调
    };
    TcpConnectionSlot::ptr slot = make_shared<AbstractSlot>(
        shared_from_this(), cb
    );
    m_weak_slot = slot;
    m_tcp_svr->freshTcpConnection(slot);
}

// 连接有活动时刷新
void TcpConnection::input() {
    // 读取数据后...
    if (now - getLastActiveTime() >= timeWheel->getInterval()) {
        TcpConnectionSlot::ptr slot = m_weak_slot.lock();
        if (slot) {
            m_tcp_svr->freshTcpConnection(slot);
        }
    }
    updateLastActiveTime();
}
```

### 7.7 时间轮优势

| 操作 | 时间复杂度 | 说明 |
|------|-----------|------|
| 添加连接 | O(1) | 直接放入槽位 |
| 刷新连接 | O(1) | 放入上一个槽位 |
| 超时检测 | O(1) | 只清空当前槽位 |
| 删除连接 | O(1) | Slot析构自动处理 |

**与其他方案对比：**

| 方案 | 添加 | 刷新 | 超时检测 | 缺点 |
|------|------|------|----------|------|
| 多定时器 | O(logN) | O(logN) | O(N) | 10000连接=10000定时器 |
| 有序队列 | O(logN) | O(logN) | O(1) | 频繁刷新效率低 |
| **时间轮** | **O(1)** | **O(1)** | **O(1)** | 适合大量连接 |

### 7.8 智能指针的妙用

```cpp
~AbstractSlot() {
    sharedPtr ptr = m_weak_ptr.lock();  // 尝试提升为 shared_ptr
    if (ptr) {
        m_cb(ptr);  // 连接还活着，执行超时回调
    }
    // 连接已销毁时，weak_ptr.lock() 返回空，不执行回调
}
```

设计精髓：
- 使用 `weak_ptr` 不影响连接生命周期
- 连接提前关闭时，不会误触发超时回调
- 只有真正超时时才执行回调

---

## 八、性能优化

### 8.1 对象池

```cpp
template <typename T>
class ObjPool {
    std::queue<T *> m_free_objs;

    T *new_obj(Args... args) {
        if (m_free_objs.empty()) {
            return new T(args...);
        }
        T *obj = m_free_objs.front();
        m_free_objs.pop();
        return obj;
    }

    void delete_obj(T *obj) {
        obj->reset();
        m_free_objs.push(obj);  // 放回池中复用
    }
};
```

### 8.2 无锁环形缓冲区

```cpp
template <typename T, int CAPACITY>
class LockFreeRingBuffer {
    std::atomic<int> m_head{0};
    std::atomic<int> m_tail{0};

    bool push(T &val);  // CAS 操作实现无锁写入
    bool pop(T &val);   // CAS 操作实现无锁读取
};
```

### 8.3 协程 vs 线程 对比

| 特性 | 线程 | 协程 |
|------|------|------|
| **调度者** | 操作系统内核 | 用户程序 |
| **切换开销** | ~1μs (陷入内核) | ~100ns (纯用户态) |
| **栈大小** | ~8MB (固定) | ~128KB (可配置) |
| **数量上限** | 几千个 | 百万级别 |
| **IO等待** | 阻塞整个线程 | 仅挂起当前协程 |

---

## 九、关键设计总结

| 设计点 | 实现方式 | 优势 |
|--------|----------|------|
| **协程切换** | 汇编 `coctx_swap` | 纯用户态切换，无内核开销 |
| **IO 多路复用** | `epoll` + 协程 | 同步编程模型，异步执行效果 |
| **线程模型** | N:1 模型 (每线程多协程) | 充分利用多核，减少锁竞争 |
| **内存管理** | 对象池 + 内存池 | 减少内存分配开销 |
| **协议支持** | 策略模式 (Codec/Dispatcher) | 易于扩展新协议 |
| **连接管理** | 时间轮 | O(1) 复杂度的超时检测 |
| **唤醒机制** | `eventfd` | 跨线程唤醒事件循环 |

---

## 十、待完成功能

- [ ] 实现非阻塞异步的 RPC 调用
- [ ] 实现 m:n 协程，采用共享协程池的模式
- [ ] 支持更多协议（如 gRPC、WebSocket）
- [ ] 服务注册与发现
- [ ] 负载均衡策略
