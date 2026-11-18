# P1 考核问答

> 形式：直接出题 → 我的回答 → 纠错。
> 状态：`✅ 已掌握` / `⚠️ 部分掌握` / `❌ 待补` / `⏳ 待回答`

---

## 第 1 轮：核心不变量

### Q1. frame 从「装着页 A、被 `WritePageGuard` 持有」变成「装着页 B」

**我的回答**

> 装页 B 就需要 `CheckedWritePage` / `FetchFrame`，BPM 在做。既然用到了页 A 所在的 frame，就是通过 `GetUsableFrame` 得到的，`Evict` 驱逐的。具体的驱逐逻辑是 ARC 策略，replacer 在做。执行 `FetchFrame` 剩下部分将 B 写入。

**纠错** `❌ 待补`

答对的：执行者是 BPM（`FetchFrame`）、`CheckedWritePage` 只是外壳、victim 由 replacer 的 ARC 策略决定、`GetUsableFrame` 是「拿一个可用帧」的封装。主线没错。

问题：答成了「谁在做什么」，但题目问的是「必须依次满足哪些前提」。三条硬前提一条没提：

1. **必须先没有人 pin 它** —— 链条第一环。`Evict()` 只从 evictable 集合里挑（`arc_replacer.cpp:43-51`），而 `evictable_ = true` 只在 `pin_count_` 归零那一刻设置（`page_guard.cpp:163-165`）。即：**guard 析构 → pin 归零 → 才可淘汰**。
2. **脏页必须先写回磁盘**，且在摘掉旧映射之前。
3. **页表映射与 frame 元数据必须先清干净**（`page_table_.erase(old_pid)` + `Reset()`），否则会出现「页表说装着 B，`data_` 里还是 A」的撕裂状态。

缺了这三条，能说出 `FetchFrame` 干了什么，但说不出**凭什么可以干**。

---

### Q1-a. 把 `page_table_.erase` 挪到 `FlushFrameLocked` 之前会怎样？

**我的回答**

> 你是说这个 `FlushFrameLocked(frames_[fid], old_pid.value()); page_table_.erase(old_pid.value());` 地方对吧。参数有 frame 和这个 pageid。如果页表先删除这个，那就不好找到 pageid。

**纠错** `⚠️ 部分掌握`

直觉对了一半（顺序确实重要），但举的机制**在代码里不成立**：

```cpp
// buffer_pool_manager.cpp:112-124
auto old_pid = frames_[fid]->page_id_;        // ← 第 113 行：已复制成本地变量

FlushFrameLocked(frames_[fid], old_pid.value());
page_table_.erase(old_pid.value());
frames_[fid]->Reset();
```

1. `old_pid` 是**局部副本**，在 `erase` 之前就取好了，先 `erase` 也不会「找不到」。
2. `FlushFrameLocked`（`:365-379`）**根本不查页表**——只接收 `frame` 和 `page_id`，直接发 `DiskRequest`。

⇒ 这题的前提本身是个陷阱：**先 `erase` 在这里并不破坏正确性**。暴露的问题是——结论靠印象，没回代码验证前提。并发 bug 恰恰都藏在这种「我以为它依赖 X」的地方。

真正不能颠倒的是另一对：`FlushFrameLocked` 必须在 `Reset()` 之前（见 Q1-c）。

---

### Q1-c. 把 `Reset()` 挪到最前面会怎样？

```cpp
frames_[fid]->Reset();                      // 先 Reset
FlushFrameLocked(frames_[fid], old_pid.value());
page_table_.erase(old_pid.value());
```

**我的回答**

> 先 reset 会触发 `page_id_ = std::nullopt`，将 `page_id_` 清空了。`FlushFrameLocked(..., page_id_t page_id)` 参数里用到这个 page_id。

**纠错** `❌` —— **同一个坑第二次**

与 Q1-a 完全同源：盯着「字段现在是什么」，而不是「这个值从哪来的」。

```cpp
// :113
auto old_pid = frames_[fid]->page_id_;   // 快照已存入局部变量
// 即使这里 Reset：page_id_ = nullopt，old_pid 仍 == A
FlushFrameLocked(frames_[fid], old_pid.value());   // 传的是 old_pid
```

`Reset()` 之后**没有任何代码再读 `frames_[fid]->page_id_`**，所以 `page_id_ = nullopt` 对这次 flush **毫无影响**。

> 关键区分：**「某字段变了」≠「某个已经在用的值变了」**。并发 bug 一半以上出在这两者的混淆上。

真正会炸的是另外两个字段：

| 字段 | 读它的地方 | 后果 |
|---|---|---|
| `is_dirty_` → `false` | `FlushFrameLocked:366` 的 `if (!frame->is_dirty_) return;` | **直接早退，一字节不写** → 页 A 修改**永久丢失** |
| `data_` → 全 0 | `:374` 的 `DiskRequest` 缓冲区 | 写回 4096 个 0 → 把页 A 内容**覆盖成垃圾** |
| `page_id_` → `nullopt` | 无人读取 | 无后果 |

（`pin_count_.store(0)` 在此路径无害：victim 本来 pin 就是 0。）

---

### Q1-c′. 如果删掉 `Reset()` 里的 `is_dirty_ = false;`，先 Reset 还安全吗？

**我的回答**

> 不安全。一是没有清空 `is_dirty_` 字段，后面根据这个变量判断的地方都会出问题。二是先 reset 那么写回的就是全零垃圾值。

**纠错** `⚠️ 结论对，一半理由错`

| 我的理由 | 判定 |
|---|---|
| ① 「没清 `is_dirty_`，后面判断会出问题」 | ❌ **反了** |
| ② 「写回全零垃圾值」 | ✅ 对，且是**唯一**致命原因 |

① 反了的理由：`is_dirty_` 保持 `true` 恰恰是**让写回得以发生**的条件（`:366` 的早退检查）；且写回成功后 `:378` 的 `frame->is_dirty_ = false;` 会自行收拾。删掉那行**只带来好处**。

真正的致命伤：`std::fill(data_.begin(), data_.end(), 0)` → flush 忠实把 4096 个 0 写上磁盘 → **页 A 内容被不可逆覆盖**。

**方法问题（比答案重要）**：「后面根据这个变量判断的地方都会出问题」是模糊归责。必须做到**字段级对应**：

> 哪个字段被改 → 哪一行代码读了它 → 产生哪个具体后果

三行对照表能写出来，才算真懂。

---

### Q1-d. 既然先 `erase` 不影响正确性，为什么仍建议 `flush → erase → Reset`？

**我的回答**

> 语义更合适吧，肯定先写回再清空重设之类更符合直觉。

**纠错** `✅`

主要理由就是「语义/直觉」——**顺序应按依赖关系排，而不是按随手写的顺序排**。三行其实在表达一个对象的生命周期：

```
保存它的内容  →  注销它的身份  →  清空它的躯壳
  (flush)        (erase)        (Reset)
```

补充两条更有分量的理由：

**① 异常安全。** `FlushFrameLocked` 里的 `future.get()` 可能抛异常（磁盘错误）。先 `erase` 一旦抛出，就同时失去**映射**和**脏数据**——想重试都找不到页在哪。保持 `flush` 在前，失败时状态至少还是自洽的。

**② 为将来的异步化留路。** 若把 flush 改成「发请求、不等结果」，提前 `erase` 会制造「页表里已没它、数据还没落盘」的窗口，重试/审计无从下手。

> 一句话：**不要提前销毁你后面还需要的信息**——哪怕「现在」看起来用不到。这也是 Q1-a 陷阱的正面版：当时以为 `page_id_` 被清空有影响（实际没有），真正该问的是「未来某条路径会不会需要它」。

---

### Q1-b. 读盘窗口里为何 frame 抢不走？

`FetchFrame` 持着 `bpm_latch_` 从磁盘读 B（`buffer_pool_manager.cpp:151-168`）。这段窗口里，为什么不可能有别的线程把这个 frame 抢走、或往里写 A/B 的数据？

**我的回答**

> `bpm_latch_` 这个锁阻止了其他线程去拿到这个 frame 进行读写。它保护 `page_table_`、`free_frames_`、frame 的页 ID / 脏标记 / pin 计数，以及 `replacer_` 的状态，确保页与 frame 的映射和 frame 的分配、回收过程不会被并发访问破坏。frame 中实际页数据的读写由 `FrameHeader::rwlatch_` 以及 page guard 保护；磁盘 I/O 由 `disk_scheduler_` 负责同步。evict 驱逐的时候将他设置成了不可淘汰的，放入了 ghost，写帧的时候 getusable 不会拿到这个。

**纠错** `✅ 掌握`

两个机制都点到了，第二个还翻对了源码。精确化：

**机制 1：`bpm_latch_` 全程持有。** 别的线程要碰这个 frame，入口只有 `FetchFrame` → `GetUsableFrame` → `Evict`，全在 `bpm_latch_` 里（`:141` 的 `scoped_lock` 活到函数结束）⇒ 窗口期内**没有任何线程能进入决策逻辑**。

**机制 2：frame 已被从 replacer 的「存活结构」里摘除。**（`arc_replacer.cpp:82-87`）

```cpp
mru_.erase(victim->alive_it_);            // 从 T1/T2 链表摘掉
mru_ghost_.insert(..., victim->page_id_);  // 记进 ghost
victim->evictable_ = false;
alive_map_.erase(victim_frame);            // 从存活表摘掉
ghost_map_[victim->page_id_] = victim;
```

「设置成不可淘汰」是**次要**的一半。**主要**是前两句：`find_victim`（`:43-51`）的工作方式是「遍历 `mru_`/`mfu_` 链表 → 对每个 id 做 `alive_map_.at()`」。链表里已经没它，连**被看见**的机会都没有；`evictable_ = false` 只是第二道保险。

> 细节：只 `alive_map_.erase` 而不从链表 `erase`，`alive_map_.at()` 会直接抛 `std::out_of_range`。两步必须成对。

---

### Q1-b′. 持锁读盘期间，访问无关页 X 会怎样？

页 X 页表里已有、且没被 pin。另一个线程此时 `ReadPage(X)`，请描述它的完整遭遇。

**我的回答**

> 每个 frame 有自己的 `rwlatch_`，是可以正常读的吧。

**纠错** `❌`

**不行，会被完全卡住。** 混淆了「数据访问」与「找到这个 frame」：

```cpp
CheckedReadPage(X)                        // :295
  → FetchFrame(X)                         // :140
      → std::scoped_lock lock(*bpm_latch_);   // :141  ← 在这里就阻塞了
          → page_table_.find(X);          // 连这一行都执行不到
```

`FetchFrame` 第一行就拿 `bpm_latch_`，**缓存命中也一样**。线程在还没搞清楚「X 在哪个 frame」时就停了——而它必须先读 `page_table_` 才知道是哪个 frame，`page_table_` 又被 `bpm_latch_` 保护。

⇒ X 与正在加载的页毫无关系、X 早在内存、X 的 `rwlatch_` 完全空闲——这个线程**照样要等一整次磁盘往返**。

> `ReadPage` 分三段，只有第 ③ 段碰得到帧锁：
> ① **查表 + pin**（`bpm_latch_`）→ ② **拿帧锁**（`frame->rwlatch_`）→ ③ 读写数据
>
> 只考虑了 ③ 能并行，卡住的是 ①。**per-frame 锁保护「数据」，保护不了「查找」**，而查找是必经之路。
>
> 这正是整个缓冲池的吞吐上限被「一次缺页 = 一次全局串行化」钉死的原因。

---

### Q1-b″. 把 I/O 挪到 `bpm_latch_` 外面，需要补什么状态？

**我的回答（第一轮）**

> 加一个变量来看是否正在从磁盘加载是可以的吗？

**纠错** `✅ 方向对，但一个 bool 只解决了三分之一`

| 问题 | bool 能解决吗 |
|---|---|
| ① 状态怎么表达（加载中 / 可用） | ✅ 就是这个 bool |
| ② 别的线程撞上 loading 之后**怎么等** | ❌ 需要条件变量 |
| ③ 页表映射**什么时候**建 | ❌ 最容易错的一点 |

关于 ②：光有 bool 只能让线程知道「不能用」，要么自旋烧 CPU、要么睡多久都不对。需配 `condition_variable`，等的是「`is_loading_` 变 false」这个**谓词**（`wait(latch, pred)`，不是 `sleep`）。

关于 atomic：`is_loading_` **不需要** atomic——所有读写都在 `bpm_latch_` 下，跟 `is_dirty_` 的教训正好相反。判据：**是不是所有访问都用同一把锁**。

**我的回答（第二轮）**

> 没有 B，因为都需要读盘了，说明没有命中内存，也就不在页表里。T2 先因为锁等待一会，之后会命中。最终内存不会出现两份 B 数据。这不都只是 Read 吗没改数据吧。要是说都又写了的话，就是写入 T2 的，因为他更晚。

**纠错** `❌` —— **第三次同一个毛病：推演的是改动前的行为**

「T2 先因为锁等待一会」是**现在**（I/O 在锁内）的样子。我们要做的改动恰恰是**让 T1 读盘时放锁**，所以 T2 根本不会等：

```
T1: [持锁] 选 victim F → 放锁 → 正在读盘 ……        ← 锁已释放
T2: [持锁] page_table_.find(B) → 没命中 → 选 victim G → 放锁 → 也去读 B
```

**T2 走了完全一样的 miss 路径，把 B 又加载了一遍。**

后果：

1. **内存里两份 B**：frame F 和 frame G 各存一份。
2. **页表被覆盖**：T1 先 `page_table_[B] = F`，T2 后 `= G`。之后只能看到 G，**F 成孤儿帧**（pinned、数据是 B、replacer 里还记着它）。
3. **replacer 记账崩坏**：两个 frame 都 `RecordAccess(fid, B, ...)`，而 `ghost_map_` 按 **`page_id`** 做键（`arc_replacer.cpp:85`），必然冲突。
4. **不一致**：T1 拿 F 的 guard、T2 拿 G 的 guard，T1 改 F、T2 读 G，**永远看不到对方的修改**。

> 「这不都只是 Read 吗」是**最危险**的误解。不一致的根源不是「谁改了」，而是**同一个页在内存里存在两份副本**。即使两边都只读，只要第三方写了其中一份，另一份就永久陈旧。
>
> **缓存正确性的前提：一个页在内存里只有一份权威副本。** 并发 miss 路径破坏的正是这一条。

正确做法：

```
① [持锁] 查表 → 未命中 → 选 victim → page_table_[B] = fid   ← 映射先建！
                                        frame->is_loading_ = true
                                        pin_count_ = 1（占位）
② [放锁] 读盘
③ [持锁] is_loading_ = false → cv.notify_all()
④ [放锁] 拿帧锁，造 guard
```

T2 在 ① 进来时 `page_table_` **命中** B → 看到 `is_loading_` → 在 ③ 的 cv 上等。**提前建映射是唯一能阻止重复加载的手段**，因为「有没有别人正在加载 B」这个信息，除了页表没有地方能全局查到。

---

**我的回答（第三轮：比喻层面推演）**

> 1. 关于 B 写了已经去仓库拿 B 了。F 这个格子是申请出来装 B 的。
> 2. T2 来查到了 B 去仓库拿了，还没拿回来，就开始睡觉，等 B 被拿回来再被叫醒。
> 3. T1 首先先把映射卡片和正在搬这个状态改掉，然后进行读写操作。T2 会被叫醒，之后拿到的也是 F 格子。
> 4. 最终内存里有 1 本 B。和改之前比，占用锁时间少吧。

**纠错** `✅ 四问全对`

两处再钉深：

① **「占用锁时间少」是手段，不是目的。** 真正收益是「跟这次 I/O 无关的访问不再被卡住」（正好是 Q1-b′ 答错的那题）。判断并发改动好不好，问的不是「锁持有得久不久」，而是「**无关的工作会不会被阻塞**」。

② **「T2 拿到 F」不是必然，只是这个时序的结果。** 机制上 T2 醒来必须**重新查一次卡片**：他睡着的这段时间里，卡片可能已经变了。

---

### Q1-b‴. T2 醒来前，B 被淘汰、F 换成了 C，会怎样？

**我的回答**

> 应该直接按从头开始 `FetchFrame` 这样查一遍。

**纠错** `✅`

若按记忆直接拿 F：**F 里现在是 C 的数据**，却被当成 B 用 → 读到别的页内容、写回污染 C，且 `page_id_` 与实际内容对不上。必须从头重查：先看 B 还在不在卡片上，再看落在哪个格子、状态是否可借。

这就是代码里 `for(;;)` + `continue` 存在的全部理由：**等待之后的世界不可信，必须重新验证**。

> 通用名字：**醒来后重新检查谓词**。`wait` 返回不代表条件成立，只代表「可能变了，你自己再看一眼」。

---

### Q1-b 小结

| 子题 | 状态 |
|---|---|
| Q1-b 两个互斥机制 | ✅ |
| Q1-b′ 无关访问为何被卡 | ❌ 答错（per-frame 锁保护不了查表阶段） |
| Q1-b″ 锁外 I/O 的设计 | ✅ 比喻层全对；但第一轮推演错（按改动前行为答） |
| Q1-b‴ 醒来后重查 | ✅ |

### Q2. `pin_count_` 和 `frame->rwlatch_` 各解决什么问题？

**我的回答**

`⏳ 待回答`

> 要求：各举一个具体场景 ——
> ① `pin_count_` 防住了但 latch 防不住的；
> ② latch 防住了但 `pin_count_` 防不住的。

---

### Q3. `Drop()` 里先 `unlock_shared()` 再拿 `bpm_latch_`

**我的回答**

`⏳ 待回答`

> 要求：
> ① 如果反过来（先拿 `bpm_latch_` 再放页锁），构造**具体死锁序列**：哪两个线程、各持什么、各等什么。
> ② 如果只是「先拿 `bpm_latch_`、放掉、再 unlock 页锁」这种**不嵌套**的顺序，还安全吗？为什么？

---

### Q4. 淘汰 victim 时不拿 `rwlatch_` 为什么安全？

`GetUsableFrame()`（`buffer_pool_manager.cpp:107`）淘汰 victim 时直接 `FlushFrameLocked` 写回，没有拿 victim 的 `rwlatch_`。

**我的回答**

`⏳ 待回答`

> 要求：
> ① 为什么不拿也安全？依赖哪条不变量？
> ② 那 `FlushPage()`（`:427`）为什么**必须**拿页写锁？两处差别在哪？

---

## 掌握清单

| 主题 | 状态 |
|---|---|
| 淘汰 victim 的三条前提 | ❌ Q1 未答出 |
| 淘汰顺序（flush / erase / Reset） | ⚠️ Q1-a：直觉对，机制错（未回代码验证前提） |
| `Reset()` 字段级后果分析 | ❌ Q1-c：再次混淆「字段值」与「快照值」 |
| 字段级因果对照（改→读→后果） | ⚠️ Q1-c′：结论对，模糊归责 |
| 淘汰顺序的防御性理由 | ✅ Q1-d（语义直觉对，补充异常安全/异步化） |
| 持锁读盘期间的互斥来源 | ✅ Q1-b（两个机制均答对，机制 2 的次要/主次关系待记） |
| 持锁 I/O 的代价 | ❌ Q1-b′：混淆数据访问与查表阶段 |
| 锁外 I/O 需要的新状态 | ✅ Q1-b″/Q1-b‴：提前建映射 + loading 状态 + 睡等 + 醒来重查 |
| 「一页只有一份权威副本」不变量 | ⚠️ 经比喻讲解后理解（首次作答认为「只是 Read 就没事」） |
| `pin_count_` vs `rwlatch_` 分工 | ⏳ Q2 |
| `Drop()` 加锁顺序与死锁链条 | ⏳ Q3 |
| `GetUsableFrame` vs `FlushPage` 的锁差异 | ⏳ Q4 |
