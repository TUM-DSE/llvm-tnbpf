#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tracepoint/syscalls/sys_enter_execve")
int hello_world(void *ctx)
{
    for (int i=0;i<5000;i++) {
      bpf_printk("hello world\n");
    }
    return 0;
}

char LICENSE[] SEC("license") = "GPL";

