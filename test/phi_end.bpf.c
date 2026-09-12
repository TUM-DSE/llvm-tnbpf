//
// Created by deniz on 9/8/26.
//
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tracepoint/syscalls/sys_enter_execve")
int hello_world(void *ctx) {
  unsigned int *unk = (unsigned int *)ctx;
  unsigned int a;
  a = unk[0];
  int n = 10;
  if (a) {
    bpf_printk("branch side effect\n");
    n = 20;
  }

  for (int i=a;i<n;i++) {
    bpf_printk("phi loop !=\n");
  }
  return 0;
}

char LICENSE[] SEC("license") = "GPL";

