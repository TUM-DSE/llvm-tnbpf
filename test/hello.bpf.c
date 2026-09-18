//
// Created by deniz on 9/8/26.
//
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tracepoint/syscalls/sys_enter_execve")
int hello_world(void *ctx) {
  for (int i=0;i<500;i++) {
    bpf_printk("something %d\n", i);
  }
}

char LICENSE[] SEC("license") = "GPL";