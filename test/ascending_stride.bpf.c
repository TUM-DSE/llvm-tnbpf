//
// Created by deniz on 9/8/26.
//
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tracepoint/syscalls/sys_enter_execve")
int hello_world(void *ctx) {
    for (unsigned int i=0;i<5000;i+=3) {
      bpf_printk("ascending loop < stride 3 %d\n", i);
    }
    for (unsigned int i=0;i<5000;i+=5) {
      bpf_printk("ascending loop < stride 5 %d\n", i);
    }
    return 0;
}

char LICENSE[] SEC("license") = "GPL";

