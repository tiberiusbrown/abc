int abc_benchmarks();
int abc_docs();
int abc_tests();

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    if(0 != abc_tests()) return 1;
    if(0 != abc_benchmarks()) return 1;
    if(0 != abc_docs()) return 1;
    
    return 0;
}
