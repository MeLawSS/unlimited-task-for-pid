//
//  unlimited_task_for_pid.c
//  unlimited-task-for-pid
//
//  Created by melo on 2024/9/23.
//

#include <mach/mach_types.h>
#include <mach/task.h>
#include <mach/vm_map.h>
#include <mach/mach_vm.h>
#include <vm/vm_kern.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/vnode_if.h>
#include <sys/fcntl.h>
#include <libkern/libkern.h>
#include <sys/kauth.h>
#include <sys/uio.h>
#include <kern/locks.h>
#include <sys/sysctl.h>
#include <kern/task.h>
#include <mach/mach_port.h>

// worked on macOS14.5 x86_64(VM)

kern_return_t unlimited_task_for_pid_start(kmod_info_t * ki, void *d);
kern_return_t unlimited_task_for_pid_stop(kmod_info_t *ki, void *d);

uint64_t symbol_slide;

task_t (*test_proc_task)(proc_t proc);
ipc_port_t (*test_convert_task_to_port)(task_t task);
mach_port_name_t (*test_ipc_port_copyout_send)(
    ipc_port_t      sright, /* can be invalid */
    ipc_space_t     space);
void (*test_extmod_statistics_incr_task_for_pid)(task_t target);
ipc_space_t (*test_get_task_ipcspace)(task_t t);
proc_t (*test_proc_find)(int pid);

// return specific pid's task's taskport
static int sysctl_get_task_port SYSCTL_HANDLER_ARGS {

    
    int error = 0;
    int pid;
    proc_t p;
    task_t task = NULL;
    mach_port_name_t tret = MACH_PORT_NULL;
    void *sright;
    
    if (req->newptr == 0) {
        printf("no input from userland!");
        error = EINVAL;
        return error;
    }
    
    if (req->newlen != sizeof(int)) {
        printf("invalid length of userland buffer!");
        error = EINVAL;
        return error;
    }
    
    printf("get userland input! copyin ret: %d, pid:%d", copyin(req->newptr, &pid, req->newlen), pid);
    
    p = proc_find(pid);
    proc_rele(p);
    
    task = test_proc_task(p);
    task_reference(task);

    /* Grant task port access */
    test_extmod_statistics_incr_task_for_pid(task);

    /* this reference will be consumed during conversion */
    task_reference(task);

    if (task == current_task()) {
        task_deallocate(task);
        proc_rele(p);
        error = EINVAL;
        return error;
    } else {
        sright = (void *)test_convert_task_to_port(task);
    }
    /* extra task ref consumed */

    if (p != NULL) {
        proc_rele(p);
    }
    p = NULL;

    tret = test_ipc_port_copyout_send(sright, test_get_task_ipcspace(current_task()));

    printf("tret = %d", tret);
    sysctl_handle_int(oidp, &tret, 0, req);

    if (task != TASK_NULL) {
        task_deallocate(task);
    }
    if (p != NULL) {
        proc_rele(p);
    }

    sysctl_handle_int(oidp, &tret, 0, req);
    
    return error;
}

SYSCTL_PROC(
    _kern,
    OID_AUTO,
    unlimited_task_for_pid,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_ANYBODY,
    NULL,
    0,
    sysctl_get_task_port,
    "I",
    "unlimited task for pid"
);

#define KERNEL_BASE 0xffffff8000200000

kern_return_t unlimited_task_for_pid_start(kmod_info_t * ki, void *d)
{
    // the "slide" of these symbols is not equal to vm_kernel_slide.
    // so we need to get this "slide" by making slided address substract original address in kernel binary.
    // I simply get these addresses with MachOView and typed them here...
    // of course, different version kernel binary has different address, it's much better to get these addresses dynamically in code.
    symbol_slide = (uint64_t)printf - 0xFFFFFF8000363830;
    
//    vm_offset_t slide;
//    vm_kernel_unslide_or_perm_external(KERNEL_BASE, &slide);
//    slide = KERNEL_BASE - slide;
//    vm_offset_t unslide_printf;
//    vm_kernel_unslide_or_perm_external((uint64_t)printf, &unslide_printf);
//    vm_offset_t extra_slide = unslide_printf - 0xFFFFFF8000363830;
//    symbol_slide = slide + extra_slide;
    
    test_proc_task = (void*)(0xFFFFFF800083A2A0 + symbol_slide);
    test_convert_task_to_port = (void*)(0xFFFFFF800034F170 + symbol_slide);
    test_ipc_port_copyout_send = (void*)(0xFFFFFF8000325EF0 + symbol_slide);
    test_extmod_statistics_incr_task_for_pid = (void*)(0xFFFFFF8000344AC0 + symbol_slide);
    test_get_task_ipcspace = (void*)(0xFFFFFF80003BDCD0 + symbol_slide);
    
    
    sysctl_register_oid(&sysctl__kern_unlimited_task_for_pid);
    
    return KERN_SUCCESS;
}

kern_return_t unlimited_task_for_pid_stop(kmod_info_t *ki, void *d)
{
    return KERN_SUCCESS;
}
