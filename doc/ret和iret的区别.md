`ret` 和 `iret` 都是“返回”，但返回的上下文完全不同。

`ret` 用于**普通函数返回**。比如：

```asm
call foo
```

CPU 会把下一条指令地址压栈，然后跳到 `foo`。`foo` 最后：

```asm
ret
```

本质上就是从栈里取出返回地址，恢复到 `EIP`：

```text
call
↓
保存返回 EIP
↓
执行函数
↓
ret
↓
恢复 EIP
```

所以可以粗略理解：

```text
ret ≈ 恢复 EIP
```

而 `iret` 是 **Interrupt Return**，用于中断、异常返回。CPU 进入中断时不仅要记住执行到哪，还要记住代码段和标志位，所以会自动保存：

```text
EIP
CS
EFLAGS
```

因此：

```asm
iret
```

会恢复：

```text
EIP
CS
EFLAGS
```

如果发生了特权级切换，比如：

```text
Ring 3 → Ring 0
```

CPU 进入内核时还会保存用户态的：

```text
ESP
SS
```

所以此时 `iret` 会恢复：

```text
EIP
CS
EFLAGS
ESP
SS
```

并重新切回 Ring 3。

可以这样记：

| 指令 | 用途 | 恢复内容 |
|---|---|---|
| `ret` | 普通函数返回 | `EIP` |
| `iret` | 中断/异常返回 | `EIP + CS + EFLAGS` |
| `iret` 跨 Ring 返回 | Ring0 → Ring3 等 | `EIP + CS + EFLAGS + ESP + SS` |

这也是为什么你现在的任务调度器最后必须：

```asm
popa
iret
```

而不是：

```asm
popa
ret
```

因为任务现场里不只有“函数返回地址”，还包含当时的 `CS`、`EFLAGS`，以后用户任务还可能包含 `SS/ESP`。`iret` 才能把完整的 CPU 执行现场恢复出来。