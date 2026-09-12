//
// Created by deniz on 9/8/26.
//
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tracepoint/syscalls/sys_enter_execve")
int hello_world(void *ctx) {
  unsigned int bound = 0x80000001;
  for (int i = 0;i<bound;i++) {
    bpf_printk("No idea what happens in this case\n");
  }
  return 0;
}

char LICENSE[] SEC("license") = "GPL";

