//
// Created by deniz on 9/18/26.
//
//
// Created by deniz on 9/8/26.
//
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tracepoint/syscalls/sys_enter_execve")
int hello_world(void *ctx) {
  for (int i=0;i<100;i++) {
      for (int j=0;j<100;j++) {
           bpf_printk("%d %d\n", i, j);
      }
  }
  return 0;
}

char LICENSE[] SEC("license") = "GPL";

