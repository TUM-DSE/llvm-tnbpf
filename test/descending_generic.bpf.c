//
// Created by deniz on 9/8/26.
//
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tracepoint/syscalls/sys_enter_execve")
int hello_world(void *ctx) {
  for (unsigned int i=5000;i>0;i--) {
    bpf_printk("descending loop, terminates %d\n", i);
  }
  return 0;
}

char LICENSE[] SEC("license") = "GPL";

