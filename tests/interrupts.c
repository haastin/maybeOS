
void divide_by_zero_excep_test(){
    //add something in this func/overall handler code to indicate that an itnerrupt handler has been handled
    asm volatile("mov $1, %%eax\n"
                "xor %%edx, %%edx\n"
                "mov $0, %%ebx\n"
                "div %%ebx\n" ::);
}
