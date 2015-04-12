#include "assembler.h"

#include <iostream>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [INPUT]" << std::endl;
        return 1;
    }

    Assembler as;
    bool ret = as.assembleFile(argv[1]);
    return ret ? 0 : -1;
}
