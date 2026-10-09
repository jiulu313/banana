# banana OS 最近 3 次会话知识点整理

> 项目：banana OS  
> 目标：通过手写 x86 32 位内核，系统掌握操作系统原理  
> 环境：Ubuntu 26 + NASM + GCC `-m32` + GNU ld + QEMU  
> 本文按最近三阶段会话整理：**Ring 3 / TSS / GDT → Syscall / 用户拷贝 → 抢占式任务调度**

---

# 会话一：Ring 3、GDT、TSS 与用户态切换

## 1. GDT 段描述符

32 位保护模式下，一个普通 GDT 段描述符占：

```text
8 bytes = 64 bits
```

核心包含四类信息：

```text
Base    段从哪里开始
Limit   段有多大
Access  类型、权限、DPL
Flags   粒度、32/64 位属性等
```

banana 当前使用 flat memory model：

```text
base  = 0
limit ≈ 4GB
```

因此分段机制主要负责：

- Ring 权限
- code / data 类型
- TSS 等 system descriptor

真正的地址隔离主要交给 Paging。

### 常用 Access Byte

| 值 | 含义 |
|---|---|
| `0x9A` | Ring 0 kernel code |
| `0x92` | Ring 0 kernel data |
| `0xFA` | Ring 3 user code |
| `0xF2` | Ring 3 user data |
| `0x89` | 32-bit available TSS |

---

## 2. 段选择子 Selector

选择子不只是 GDT 下标，它的格式大致是：

```text
15                         3  2   1 0
+---------------------------+----+----+
|          Index            | TI |RPL |
+---------------------------+----+----+
```

当前未使用 LDT，所以：

```text
selector = (GDT index << 3) | RPL
```

banana 的典型选择子：

| 用途 | GDT Index | RPL | Selector |
|---|---:|---:|---:|
| kernel code | 1 | 0 | `0x08` |
| kernel data | 2 | 0 | `0x10` |
| user code | 3 | 3 | `0x1B` |
| user data | 4 | 3 | `0x23` |
| TSS | 5 | 0 | `0x28` |

例如：

```text
0x1B >> 3 = 3
```

说明它选择 `GDT[3]`。

---

## 3. 为什么内核栈使用 `SS = 0x10`

`0x08` 是 kernel code segment。

`0x10` 是 kernel data segment。

栈本质上是一块可读写数据区：

```asm
push eax
```

可以粗略理解成：

```text
ESP -= 4
[SS:ESP] = EAX
```

所以：

```text
CS = 0x08    kernel code
DS = 0x10    kernel data
SS = 0x10    kernel stack
```

TSS 中因此使用：

```c
tss.ss0 = 0x10;
```

---

## 4. TSS 的作用

现代 32 位 banana 内核并不依赖 TSS 做完整的硬件任务切换。

我们主要使用：

```text
SS0
ESP0
```

它们告诉 CPU：

> 从 Ring 3 进入 Ring 0 时，应该切换到哪个内核栈。

流程：

```text
Ring 3
SS  = 0x23
ESP = user_stack

    ↓ int 0x80 / exception / IRQ

CPU 查 TR
    ↓
找到 TSS
    ↓
读取 SS0 / ESP0
    ↓
切换到 kernel stack
    ↓
执行 Ring 0 ISR
```

TSS descriptor 放在：

```text
GDT[5]
```

所以：

```text
TSS selector = 5 << 3 = 0x28
```

通过：

```asm
mov ax, 0x28
ltr ax
```

加载 Task Register。

---

## 5. Ring 0 → Ring 3

不能直接：

```asm
mov cs, 0x1B
```

来切换 CPL。

经典做法是手工构造 `iret` 栈帧：

```asm
push dword 0x23      ; user SS
push user_esp
pushfd
or dword [esp], 0x200
push dword 0x1B      ; user CS
push user_eip
iret
```

`iret` 前：

```text
ESP -> user EIP
       user CS
       EFLAGS
       user ESP
       user SS
```

CPU 看到：

```text
CS = 0x1B
RPL = 3
```

并且对应 user code descriptor：

```text
DPL = 3
```

于是发生：

```text
CPL 0 → CPL 3
```

最终：

```text
CS  = 0x1B
SS  = 0x23
EIP = user_entry
ESP = user_stack
CPL = 3
```

---

# 会话二：系统调用、syscall frame 与用户内存安全

## 1. `int 0x80` 作为 syscall 入口

约定 ABI：

```text
EAX = syscall number
EBX = arg1
ECX = arg2
EDX = arg3

返回值 = EAX
```

为了让 Ring 3 主动调用：

```asm
int 0x80
```

IDT gate 必须允许 DPL=3，例如：

```c
idt_set_gate(
    0x80,
    (unsigned int)isr80,
    0x08,
    0xEE
);
```

`0xEE` 表示：

```text
Present = 1
DPL     = 3
Type    = 32-bit interrupt gate
```

ISR 自身仍然通过：

```text
selector = 0x08
```

进入 Ring 0。

---

## 2. `isr80`

当前结构：

```asm
isr80:
    pusha

    mov eax, esp
    push eax

    call syscall_handler

    add esp, 4

    popa
    iret
```

核心逻辑：

```text
pusha
↓
保存用户进入 syscall 时的通用寄存器

mov eax, esp
↓
EAX = 保存现场的起始地址

push eax
↓
把 frame 地址作为 C 函数第一个参数压栈

call syscall_handler
↓
进入 C 内核

popa
↓
恢复通用寄存器

iret
↓
恢复用户 EIP / CS / EFLAGS / ESP / SS
```

---

## 3. 为什么 `mov eax, esp` 不会丢失 syscall number

用户进入 syscall 时：

```text
EAX = syscall number
```

先执行：

```asm
pusha
```

后，旧 EAX 已经保存到栈中。

随后：

```asm
mov eax, esp
```

确实覆盖了当前 CPU 的 EAX，但：

```text
saved EAX
```

仍然保存在栈中。

所以 C 中：

```c
frame->eax
```

读到的是 `pusha` 保存的旧 EAX，而不是当前临时 EAX。

---

## 4. syscall_frame 并不是实际创建的 C 变量

结构：

```c
struct syscall_frame
{
    unsigned int edi;
    unsigned int esi;
    unsigned int ebp;
    unsigned int esp;
    unsigned int ebx;
    unsigned int edx;
    unsigned int ecx;
    unsigned int eax;
};
```

并没有：

```c
struct syscall_frame frame;
```

这样的真实变量。

而是：

> 栈上的原始字节布局刚好与结构体字段布局一致，于是把栈顶地址解释成 `struct syscall_frame *`。

也就是：

```text
已有的栈内存
↓
强制按 syscall_frame 布局解释
↓
frame->eax
frame->ebx
...
```

---

## 5. syscall 返回值

如果 C 中：

```c
frame->eax = return_value;
```

就是修改了栈中保存的 EAX 槽位。

之后：

```asm
popa
```

会把这个新值恢复到真正的 CPU EAX。

所以：

```text
Ring 3:
EAX = syscall number

↓ int 0x80

Ring 0:
frame->eax = result

↓ popa + iret

Ring 3:
EAX = syscall return value
```

---

## 6. 用户指针不能直接信任

错误方式：

```c
vga_write((char *)frame->ebx);
```

因为 Ring 3 可以传：

```text
0xDEADBEEF
```

或内核地址。

正确思路：

```text
用户指针
↓
检查页表
↓
PDE Present/User
↓
PTE Present/User
↓
必要时检查 Write
↓
再访问
```

逐步实现：

```c
is_user_address_mapped()
is_user_address_writable()
copy_from_user()
copy_string_from_user()
copy_to_user()
```

这样形成明确的信任边界：

```text
Ring 3：不可信
    ↓ syscall
Ring 0：检查、复制、处理
```

---

# 会话三：抢占式调度、任务栈与上下文切换

## 1. 调度器核心思想

任务切换本质：

```text
保存当前任务上下文
↓
选择下一个任务
↓
恢复下一个任务上下文
```

在 banana 当前设计中，完整寄存器现场主要保存在任务自己的栈上，因此 task 结构最关键的是：

```c
unsigned int esp;
```

因为：

> ESP 指向哪里，就等于选择了哪一份保存好的 CPU 上下文。

---

## 2. Timer IRQ 驱动抢占

IRQ0：

```text
PIT
↓
IRQ0
↓
pusha
↓
scheduler_on_tick(current_esp)
↓
返回 next_task_esp
↓
mov esp, eax
↓
popa
↓
iret
```

关键操作：

```asm
mov esp, eax
```

执行前：

```text
ESP -> task A stack
```

执行后：

```text
ESP -> task B stack
```

随后：

```asm
popa
iret
```

自然恢复 B 的寄存器、EIP、CS、EFLAGS。

这就是“切栈 = 切任务”的核心。

---

## 3. `prepare_task_stack()`

新任务从未运行过，没有真实中断现场。

因此我们提前伪造一份：

```c
static unsigned int prepare_task_stack(
    unsigned char *stack,
    void (*entry)(void))
{
    unsigned int *sp =
        (unsigned int *)(stack + STACK_SIZE);

    *--sp = 0x202;                 // EFLAGS
    *--sp = 0x08;                  // CS
    *--sp = (unsigned int)entry;   // EIP

    *--sp = 0;   // EAX
    *--sp = 0;   // ECX
    *--sp = 0;   // EDX
    *--sp = 0;   // EBX
    *--sp = 0;   // ESP dummy
    *--sp = 0;   // EBP
    *--sp = 0;   // ESI
    *--sp = 0;   // EDI

    return (unsigned int)sp;
}
```

这个函数建议放：

```text
kernel/task.c
```

或早期教学版本放在：

```text
kernel/scheduler.c
```

通用化之后放到 `task.c` 更合理。

---

## 4. `prepare_task_stack()` 执行后的栈布局

从当前 ESP 往高地址看：

```text
低地址

ESP ->  EDI = 0
        ESI = 0
        EBP = 0
        ESP dummy = 0
        EBX = 0
        EDX = 0
        ECX = 0
        EAX = 0
        EIP = entry
        CS = 0x08
        EFLAGS = 0x202

高地址
```

这份栈被设计成：

```text
前 8 个槽位 → 给 popa
后 3 个槽位 → 给 iret
```

---

## 5. 为什么 `popa` 只弹 8 个槽位

`popa` / `popad` 的硬件定义就是恢复：

```text
EDI
ESI
EBP
跳过 saved ESP
EBX
EDX
ECX
EAX
```

一共：

```text
8 × 4 = 32 bytes
```

它不知道、也不会处理：

```text
EIP
CS
EFLAGS
```

因此：

```text
执行 popa 前：

ESP -> EDI
       ...
       EAX
       EIP
       CS
       EFLAGS
```

执行 `popa` 后：

```text
ESP += 32
```

刚好：

```text
ESP -> EIP
       CS
       EFLAGS
```

然后：

```asm
iret
```

继续恢复：

```text
EIP
CS
EFLAGS
```

这两个指令分工非常明确：

```text
popa
→ 恢复通用寄存器

iret
→ 恢复中断返回现场
```

---

## 6. `ret` 与 `iret`

### `ret`

用于普通函数：

```text
call
↓
保存返回地址
↓
函数执行
↓
ret
↓
恢复 EIP
```

可以粗略理解为：

```text
ret ≈ 恢复普通返回地址
```

### `iret`

用于中断/异常：

同 Ring 时恢复：

```text
EIP
CS
EFLAGS
```

跨 Ring 时还恢复：

```text
ESP
SS
```

所以：

| 指令 | 用途 | 恢复内容 |
|---|---|---|
| `ret` | 普通函数返回 | EIP |
| `iret` | 中断返回 | EIP + CS + EFLAGS |
| `iret` 跨 Ring | 特权级返回 | EIP + CS + EFLAGS + ESP + SS |

---

## 7. 第一次调度的 Bootstrap 问题

最初 CPU 还在：

```text
kernel_main
```

并不属于 task A / task B。

所以不能：

```c
current_task = 0;
```

否则第一次 IRQ 会把 `kernel_main` 的 ESP 覆盖到：

```text
task[0].esp
```

导致 A 的预制初始栈丢失。

正确方式：

```c
static int current_task = -1;
```

第一次 tick：

```text
kernel_main
↓
IRQ0
↓
current_task == -1
↓
不保存 bootstrap ESP
↓
直接切到 task A 的预制 ESP
```

之后：

```text
A → B → A → B ...
```

这一点已经通过“最初全是 B，修改后 A/B 正常切换”的实际 bug 验证过。

---

## 8. 通用化任务创建

从：

```c
static unsigned char task_stack_a[4096];
static unsigned char task_stack_b[4096];
```

升级到：

```c
task_create(entry);
scheduler_add_task(task);
```

`task_create()` 的职责：

```text
申请 task slot
↓
PMM 申请一页 kernel stack
↓
prepare_task_stack()
↓
保存 task->esp
↓
task state = READY
```

调度器不再认识 `task_a/task_b`，只认识：

```c
struct task *
```

这是从“写死演示代码”走向真正任务系统的重要一步。

---

# 三次会话串起来后的整体理解

最近三次会话实际上把之前做过的多个内核模块真正串起来了：

```text
GDT / Ring 3
      ↓
TSS 提供 Ring 3 → Ring 0 内核栈
      ↓
int 0x80
      ↓
syscall frame
      ↓
copy_from_user / copy_to_user
      ↓
PIT IRQ0
      ↓
保存 CPU 上下文
      ↓
切换 ESP
      ↓
popa + iret
      ↓
任务调度
```

因此目前 banana 已经拥有现代 OS 的几个最关键骨架：

```text
内核态 / 用户态
系统调用
虚拟内存
物理页管理
硬件中断
抢占式任务切换
```

---

# 下一阶段建议

接下来可以继续沿着任务/进程方向：

1. 完善通用 `task_create()`。
2. 每个 task 使用独立 kernel stack。
3. 调度时同步更新 `TSS.ESP0`。
4. 把 Ring 3 用户任务接入 scheduler。
5. 每个用户进程建立独立 page directory / CR3。
6. 实现真正的 process address space。
7. 增加 `sleep / wakeup / blocked` 状态。
8. 扩展 syscall。
9. 加载用户 ELF。
10. 继续做文件系统与用户态 shell。