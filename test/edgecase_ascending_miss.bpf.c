//
// Created by deniz on 9/8/26.
//
//
// Created by deniz on 9/8/26.
//
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tracepoint/syscalls/sys_enter_execve")
int hello_world(void *ctx) {
  for (unsigned int i=0;i<=0xfffffffe;i+=2) {
    bpf_printk("ascending loop, does not terminate\n");
  }
  return 0;
}

char LICENSE[] SEC("license") = "GPL";

