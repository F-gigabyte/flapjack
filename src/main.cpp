#include <terminal.h>
#include <cstdio>

int main(int argc, const char* argv[])
{
    if(argc == 0)
    {
        std::fprintf(stderr, "Usage: flapjack [file]\r\nNo arguments were provided when the first should be this executable\r\n");
    }
    else if(argc > 2)
    {
        std::fprintf(stderr, "Usage: %s [file]\r\n", argv[0]);
    }
    Terminal terminal(argv[0]);
    if(argc == 1)
    {
        terminal.run_cmdline();
    }
    else
    {
        terminal.run_file(argv[1]);
    }
    return 0;
}
