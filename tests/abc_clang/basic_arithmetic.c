int basic_sum(int x, int y)
{
    return x + y;
}

int array_sum(int* x, int n)
{
    int t = 0;
    for(int i = 0; i < n; ++i)
        t += x[i];
    return t;
}

int array_sum_prog(int __prog* x, int n)
{
    int t = 0;
    for(int i = 0; i < n; ++i)
    {
        t += x[i];
    }
    return t;
}

int main()
{
    return 0;
}
