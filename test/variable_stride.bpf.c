//
// Created by deniz on 9/13/26.
//
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tracepoint/syscalls/sys_enter_execve")
int hello_world(void *ctx) {
  int *unk = (int *)(ctx);
  for (int i=0;i<20;) {
    bpf_printk("side effect %d\n", i);
    if (unk[i] & 1) {
      i++;
    } else {
      i += 2;
    }
  }
  return 0;
}

char LICENSE[] SEC("license") = "GPL";

