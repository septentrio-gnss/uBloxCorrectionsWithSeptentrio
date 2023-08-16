#include "program_option.hpp"
#include "ssnppl.hpp"

int main(int argc, char *argv[])
{
    Ssnppl_demonstrator ssnppl;

    if (ssnppl.init(argc, argv) == ssnppl_error::SUCCESS)
        ssnppl.dispatch_forever();

    return 0;
}