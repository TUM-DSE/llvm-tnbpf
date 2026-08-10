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
    */
    for (int i=5000;i>=0;i-=3) {
      bpf_printk("descending loop >= stride 3\n");
    }
    /*
    for (int i=0;i!=5000;i++) {
      bpf_printk("ascending loop !=\n");
    }

    for (int i=0;i!=5000;i+=3) {
      bpf_printk("ascending loop != stride 3 (bad bounds!)\n");
    }
    */
  /*
    unsigned int *unk = (unsigned int *)ctx;
    unsigned int a;
    unsigned int b;
    a = unk[0];
    b = unk[1];

    int n = 10;
    if (a) {
      bpf_printk("branch side effect\n");
      n = 20;
    }
    for (int i=a;i<n;i++) {
      bpf_printk("phi loop !=\n");
    }
    for (int i=b;i<500;i++) {
      bpf_printk("other instruction bound loop\n");
    }
    for (int i=b;i<125;i++) {
      bpf_printk("duplicate instruction bound loop\n");
    }
    */

    return 0;
}

char LICENSE[] SEC("license") = "GPL";

