#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "instructions.h"

#include <memory>

class Assembler
{
public:
    Assembler();
    ~Assembler();

    bool assembleFile(const std::string &path);

private:
    struct ASMLine {
        unsigned int lineNumber;
        std::string line;
        unsigned int location;
        unsigned int locationNext;
        std::string label;
        std::string instr;
        std::vector<std::string> params;
        int objectCode;
    };

    void error(ASMLine *asmLine, const char *fmt, ...);

    bool pass1(const std::vector<std::string> &lines);

    bool pass2();

    bool writeOutput(const std::string &listingFile,
                     const std::string &objectFile);

    void splitString(std::vector<std::string> *out, const std::string &in, char delim);

    bool isNumber(const std::string &str);

    int findLabelAddr(const std::string &label);

    bool convertMovToSicXE(const std::vector<std::string> &params,
                           bool extended,
                           std::string *instrOut,
                           std::vector<std::string> *paramsOut);
    bool convertLdStToSicXE(const std::string &instr,
                            const std::vector<std::string> &params,
                            bool extended,
                            std::string *instrOut,
                            std::vector<std::string> *paramsOut);
    void convertIndexing(std::vector<std::string> *params);

    std::string getQuoted(const std::string &str);
    int hexCharToInt(unsigned char c);

    std::string getObjCodeStr(ASMLine *asmLine);

    int getObjCode1Byte(const Instructions::InstrInfo *info);
    int getObjCode2Bytes(const Instructions::InstrInfo *info,
                         const std::string &reg1, const std::string &reg2);
    int getObjCode3Or4Bytes(const Instructions::InstrInfo *info,
                            const std::string &instr,
                            const std::vector<std::string> &params,
                            int prog, int base);

    int getRelativeAddr(int prog, int base, int target,
                        bool *useProg, bool *useBase, int *addr);

    std::string m_path;
    std::vector<ASMLine *> m_lines;
    Instructions m_instrs;
    int m_loc;
    int m_base;
    int m_start;
    std::string m_name;
    std::string m_error;
};

#endif // ASSEMBLER_H
