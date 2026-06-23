#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tracepoint/syscalls/sys_enter_execve")
int hello_world(void *ctx) {
  /*
    for (int i=0;i<5000;i++) {
      bpf_printk("ascending loop <\n");
    }
    for (int i=0;i<=5000;i++) {
      bpf_printk("ascending loop <=\n");
    }
    for (int i=5000;i>0;i--) {
      bpf_printk("descending loop >\n");
    }
    for (int i=5000;i>=0;i--) {
      bpf_printk("descending loop >=\n");
    }

    for (int i=0;i<5000;i+=3) {
      bpf_printk("ascending loop < stride 3\n");
    }
    for (int i=0;i<=5000;i+=3) {
      bpf_printk("ascending loop <= stride 3\n");
    }
    for (int i=5000;i>0;i-=3) {
      bpf_printk("descending loop > stride 3\n");
    }
    for (int i=5000;i>=0;i-=3) {
      bpf_printk("descending loop >= stride 3\n");
    }

    for (int i=0;i!=5000;i++) {
      bpf_printk("ascending loop !=\n");
    }

    for (int i=0;i!=5000;i+=3) {
      bpf_printk("ascending loop != stride 3 (bad bounds!)\n");
    }
    */
    unsigned int *unk = (unsigned int *)ctx;
    unsigned int a, b;
    a = unk[0];
    b = unk[1];
    for (unsigned int i=0;i<a && i < 500;i++) {
      bpf_printk("variant loop\n");
    }

    return 0;
}

char LICENSE[] SEC("license") = "GPL";

