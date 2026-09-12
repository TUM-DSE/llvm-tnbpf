//
// Created by deniz on 9/8/26.
//
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tracepoint/syscalls/sys_enter_execve")
int hello_world(void *ctx) {
  for (unsigned int i=5;i>0;i-=2) {
    bpf_printk("5 not even, this will never be exactly zero\n");
  }
  return 0;
}

char LICENSE[] SEC("license") = "GPL";

