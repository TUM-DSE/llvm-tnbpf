//
// Created by deniz on 9/8/26.
//
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tracepoint/syscalls/sys_enter_execve")
int hello_world(void *ctx) {
  unsigned int bound = 0x80000001;
  for (int i = 0;i<bound;i++) {
    bpf_printk("Infinite loop, i will overflow %d\n", i);
  }
  return 0;
}

char LICENSE[] SEC("license") = "GPL";

