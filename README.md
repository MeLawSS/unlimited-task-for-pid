# unlimited-task-for-pid
implement task_for_pid in a "sysctl" way that omit all security check within task_for_pid

userland task_for_pid and sysctl have similar implementation:__asm { syscall; Low latency system call }, 
so I have an idea of simulate a task_for_pid throught sysctl

kernel task_for_pid implementation:

https://github.com/apple-oss-distributions/xnu/blob/94d3b452840153a99b38a3a9659680b2a006908e/bsd/vm/vm_unix.c#L940

I simply delete all extra security check and audit code and move the rest into my syctl handler.


I had tested this program in MacOS 14.5 x64(VM), injected dylib into launchd and TextEdit.
symbol addresses in code is hardcoded, different version kernel binarys have different symbol addresses.
if you want to try this program on your machine, you need to get needed symbol addresses in your kernel binary.
(of course even better to extract these addresses automatically in code, I'm too lazy to do it)

I'm not sure whether the way of ASLR is same in 10.15, 11, 12, 13, 14.(In MacOS 14, the gap between symbol address and "slided" symbol address is slightly different from _vm_kernel_slide_)
but I believe there's a way to resolve private kernel symbol in x64 system.


**this trick is not avaliable on apple processor machine due to Pointer Authentication Codes(PAC).**


injector code originates from here↓

https://knight.sc/malware/2019/03/15/code-injection-on-macos.html
https://gist.github.com/knightsc/45edfc4903a9d2fa9f5905f60b02ce5a

get the idea of resolving private kernel symbol from here↓(the way described in this page is not avaliable in MacOS 14)
https://www.zdziarski.com/blog/?p=6901
