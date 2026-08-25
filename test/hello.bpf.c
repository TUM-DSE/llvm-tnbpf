#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tracepoint/syscalls/sys_enter_execve")
int hello_world(void *ctx) {

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
  /*

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

    // this test case is to have the phi node reference multiple instructions
    /*
    int i = 0;
    unsigned int *unk = (unsigned int *)ctx;
    unsigned int a;
    a = unk[0];

    if (a) {
      bpf_printk("branch side effect\n");
      i = 20;
    }
    for (;i<=5000;i++) {
      bpf_printk("loop going up %d\n", i);
    }
    */
  /*
    for (int i=30;i>=-5;i--) {
        bpf_printk("loop going down %d\n", i);
    }
  */
  /*
    unsigned int *unk = (unsigned int *)ctx;
    int pascal_array_length = unk[0];
    unsigned int *upper = &unk[1 + pascal_array_length];
    //TODO: this completely breaks LLVM's ability to even parse this loop's bounds
  // How does one approach this? Can LLVM / SCEV just not recognize GEP as an addition here?
    for (unsigned int *p = &unk[1]; p < upper; p++) {
      bpf_printk("ascending loop pascal style %d\n", *p);
    }
*/

    /*
    for (int i=0;i!=5000;i++) {
      bpf_printk("ascending loop !=\n");
    }

    for (int i=0;i!=5000;i+=3) {
      bpf_printk("ascending loop != stride 3 (bad bounds!)\n");
    }
    */

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
   /*
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

