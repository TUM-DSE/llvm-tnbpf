//
// Created by deniz on 9/13/26.
//
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tracepoint/syscalls/sys_enter_execve")
int hello_world(void *ctx) {
  int *array = (int *)(ctx);
  int *array_end = &array[50];
  for (;array < array_end; array = &array[1]) {
    bpf_printk("array loop %d\n", *array);
  }
}

char LICENSE[] SEC("license") = "GPL";

