#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <string>
#include <vector>

class Instructions
{
public:
    enum class SicXE
    {
        ADD,
        ADDF,
        ADDR,
        AND,
        CLEAR,
        COMP,
        COMPF,
        COMPR,
        DIV,
        DIVF,
        DIVR,
        FIX,
        FLOAT,
        HIO,
        J,
        JEQ,
        JGT,
        JLT,
        JSUB,
        LDA,
        LDB,
        LDCH,
        LDF,
        LDL,
        LDS,
        LDT,
        LDX,
        LPS,
        MUL,
        MULF,
        MULR,
        NORM,
        OR,
        RD,
        RMO,
        RSUB,
        SHIFTL,
        SHIFTR,
        SIO,
        SSK,
        STA,
        STB,
        STCH,
        STF,
        STI,
        STL,
        STS,
        STSW,
        SIT,
        STX,
        SUB,
        SUBF,
        SUBR,
        SVC,
        TD,
        TIO,
        TIX,
        TIXR,
        WD
    };

    static const std::string Directive_START;
    static const std::string Directive_END;
    static const std::string Directive_BASE;
    static const std::string Directive_NOBASE;
    static const std::string Variable_WORD;
    static const std::string Variable_RESW;
    static const std::string Variable_RESB;
    static const std::string Variable_BYTE;
    static const std::string Additional_MOV;

    enum class Length
    {
        One,
        Two,
        ThreeOrFour
    };

    enum class Type
    {
        ZeroOp,
        OneOp,
        TwoOp
    };

    struct InstrInfo {
        SicXE instr;
        const char *name;
        unsigned int opcode;
        Length length;
        Type type;
    };

    typedef struct InstrInfo InstrInfo;

    Instructions();

    const InstrInfo * operator[](const SicXE &instr);
    const InstrInfo * operator[](const std::string &instr);

    bool isValid(const std::string &instr);
    bool isSicXE(const std::string &instr);
    static bool isDirective(const std::string &instr);
    static bool isVariable(const std::string &instr);
    static bool isAdditional(const std::string &instr);
    static bool isExtended(const std::string &instr);
    static bool isParamIndirect(const std::vector<std::string> &param);
    static bool isParamIndirect(const std::string &label);
    static bool isParamImmediate(const std::vector<std::string> &params);
    static bool isParamImmediate(const std::string &label);
    static bool isParamIndex(const std::vector<std::string> &params);
    static std::string stripModifiers(const std::string &text);
    static int getRegister(const std::string &reg);

private:
    std::vector<InstrInfo> m_instrs;
};

#endif // INSTRUCTIONS_H
