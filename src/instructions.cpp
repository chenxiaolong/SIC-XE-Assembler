#include "instructions.h"

#include <cassert>


const std::string Instructions::Directive_START = "START";
const std::string Instructions::Directive_END = "END";
const std::string Instructions::Directive_BASE = "BASE";
const std::string Instructions::Directive_NOBASE = "NOBASE";
const std::string Instructions::Variable_WORD = "WORD";
const std::string Instructions::Variable_RESW = "RESW";
const std::string Instructions::Variable_RESB = "RESB";
const std::string Instructions::Variable_BYTE = "BYTE";
const std::string Instructions::Additional_MOV = "MOV";
const std::string Instructions::Additional_LD = "LD";
const std::string Instructions::Additional_ST = "ST";

const int Instructions::Register_A = 0;
const int Instructions::Register_X = 1;
const int Instructions::Register_L = 2;
const int Instructions::Register_B = 3;
const int Instructions::Register_S = 4;
const int Instructions::Register_T = 5;
const int Instructions::Register_F = 6;
const int Instructions::Register_PC = 8;
const int Instructions::Register_SW = 9;

Instructions::Instructions()
{
    // The assembler does not care which instruction is used. It only needs to
    // know the length (1, 2, 3/4 bytes) and the number of operands (0, 1, or 2)
    m_instrs = {
        { SicXE::ADD,    "ADD",    0x18, Length::ThreeOrFour, Type::OneOp },
        { SicXE::ADDF,   "ADDF",   0x58, Length::ThreeOrFour, Type::OneOp },
        { SicXE::ADDR,   "ADDR",   0x90, Length::Two,         Type::TwoOp },
        { SicXE::AND,    "AND",    0x40, Length::ThreeOrFour, Type::OneOp },
        { SicXE::CLEAR,  "CLEAR",  0xB4, Length::Two,         Type::OneOp },
        { SicXE::COMP,   "COMP",   0x28, Length::ThreeOrFour, Type::OneOp },
        { SicXE::COMPF,  "COMPF",  0x88, Length::ThreeOrFour, Type::OneOp },
        { SicXE::COMPR,  "COMPR",  0xA0, Length::Two,         Type::TwoOp },
        { SicXE::DIV,    "DIV",    0x24, Length::ThreeOrFour, Type::OneOp },
        { SicXE::DIVF,   "DIVF",   0x64, Length::ThreeOrFour, Type::OneOp },
        { SicXE::DIVR,   "DIVR",   0x9C, Length::Two,         Type::TwoOp },
        { SicXE::FIX,    "FIX",    0xC4, Length::One,         Type::ZeroOp },
        { SicXE::FLOAT,  "FLOAT",  0xC0, Length::One,         Type::ZeroOp },
        { SicXE::HIO,    "HIO",    0xF4, Length::One,         Type::ZeroOp },
        { SicXE::J,      "J",      0x3C, Length::ThreeOrFour, Type::OneOp },
        { SicXE::JEQ,    "JEQ",    0x30, Length::ThreeOrFour, Type::OneOp },
        { SicXE::JGT,    "JGT",    0x34, Length::ThreeOrFour, Type::OneOp },
        { SicXE::JLT,    "JLT",    0x38, Length::ThreeOrFour, Type::OneOp },
        { SicXE::JSUB,   "JSUB",   0x48, Length::ThreeOrFour, Type::OneOp },
        { SicXE::LDA,    "LDA",    0x00, Length::ThreeOrFour, Type::OneOp },
        { SicXE::LDB,    "LDB",    0x68, Length::ThreeOrFour, Type::OneOp },
        { SicXE::LDCH,   "LDCH",   0x50, Length::ThreeOrFour, Type::OneOp },
        { SicXE::LDF,    "LDF",    0x70, Length::ThreeOrFour, Type::OneOp },
        { SicXE::LDL,    "LDL",    0x08, Length::ThreeOrFour, Type::OneOp },
        { SicXE::LDS,    "LDS",    0x6C, Length::ThreeOrFour, Type::OneOp },
        { SicXE::LDT,    "LDT",    0x74, Length::ThreeOrFour, Type::OneOp },
        { SicXE::LDX,    "LDX",    0x04, Length::ThreeOrFour, Type::OneOp },
        { SicXE::LPS,    "LPS",    0xD0, Length::ThreeOrFour, Type::OneOp },
        { SicXE::MUL,    "MUL",    0x20, Length::ThreeOrFour, Type::OneOp },
        { SicXE::MULF,   "MULF",   0x60, Length::ThreeOrFour, Type::OneOp },
        { SicXE::MULR,   "MULR",   0x98, Length::Two,         Type::TwoOp },
        { SicXE::NORM,   "NORM",   0xC8, Length::One,         Type::ZeroOp },
        { SicXE::OR,     "OR",     0x44, Length::ThreeOrFour, Type::OneOp },
        { SicXE::RD,     "RD",     0xD8, Length::ThreeOrFour, Type::OneOp },
        { SicXE::RMO,    "RMO",    0xAC, Length::Two,         Type::TwoOp },
        { SicXE::RSUB,   "RSUB",   0x4C, Length::ThreeOrFour, Type::ZeroOp },
        { SicXE::SHIFTL, "SHIFTL", 0xA4, Length::Two,         Type::TwoOp },
        { SicXE::SHIFTR, "SHIFTR", 0xA8, Length::Two,         Type::TwoOp },
        { SicXE::SIO,    "SIO",    0xF0, Length::One,         Type::ZeroOp },
        { SicXE::SSK,    "SSK",    0xEC, Length::ThreeOrFour, Type::OneOp },
        { SicXE::STA,    "STA",    0x0C, Length::ThreeOrFour, Type::OneOp },
        { SicXE::STB,    "STB",    0x78, Length::ThreeOrFour, Type::OneOp },
        { SicXE::STCH,   "STCH",   0x54, Length::ThreeOrFour, Type::OneOp },
        { SicXE::STF,    "STF",    0x80, Length::ThreeOrFour, Type::OneOp },
        { SicXE::STI,    "STI",    0xD4, Length::ThreeOrFour, Type::OneOp },
        { SicXE::STL,    "STL",    0x14, Length::ThreeOrFour, Type::OneOp },
        { SicXE::STS,    "STS",    0x7C, Length::ThreeOrFour, Type::OneOp },
        { SicXE::STSW,   "STSW",   0xE8, Length::ThreeOrFour, Type::OneOp },
        { SicXE::STT,    "STT",    0x84, Length::ThreeOrFour, Type::OneOp },
        { SicXE::STX,    "STX",    0x10, Length::ThreeOrFour, Type::OneOp },
        { SicXE::SUB,    "SUB",    0x1C, Length::ThreeOrFour, Type::OneOp },
        { SicXE::SUBF,   "SUBF",   0x5C, Length::ThreeOrFour, Type::OneOp },
        { SicXE::SUBR,   "SUBR",   0x94, Length::Two,         Type::TwoOp },
        { SicXE::SVC,    "SVC",    0xB0, Length::Two,         Type::OneOp },
        { SicXE::TD,     "TD",     0xE0, Length::ThreeOrFour, Type::OneOp },
        { SicXE::TIO,    "TIO",    0xF8, Length::One,         Type::ZeroOp },
        { SicXE::TIX,    "TIX",    0x2C, Length::ThreeOrFour, Type::OneOp },
        { SicXE::TIXR,   "TIXR",   0xB8, Length::Two,         Type::OneOp },
        { SicXE::WD,     "WD",     0xDC, Length::ThreeOrFour, Type::OneOp }
    };
}

const Instructions::InstrInfo * Instructions::operator[](const SicXE &instr)
{
    return &m_instrs[static_cast<int>(instr)];
}

const Instructions::InstrInfo * Instructions::operator[](const std::string &instr)
{
    std::string instrOnly = stripModifiers(instr);

    for (auto const &instrInfo : m_instrs) {
        if (instrInfo.name == instrOnly) {
            return &instrInfo;
        }
    }

    return nullptr;
}

bool Instructions::isValid(const std::string &instr)
{
    return isSicXE(instr)
            || isDirective(instr)
            || isVariable(instr)
            || isAdditional(instr);
}

bool Instructions::isSicXE(const std::string &instr)
{
    std::string instrOnly = stripModifiers(instr);

    for (auto const &instrInfo : m_instrs) {
        if (instrInfo.name == instrOnly) {
            return true;
        }
    }

    return false;
}

bool Instructions::isDirective(const std::string &instr)
{
    return instr == Directive_START
            || instr == Directive_END
            || instr == Directive_BASE
            || instr == Directive_NOBASE;
}

bool Instructions::isVariable(const std::string &instr)
{
    return instr == Variable_WORD
            || instr == Variable_RESW
            || instr == Variable_RESB
            || instr == Variable_BYTE;
}

bool Instructions::isAdditional(const std::string &instr)
{
    return stripModifiers(instr) == Additional_MOV
            || stripModifiers(instr) == Additional_LD
            || stripModifiers(instr) == Additional_ST;
}

bool Instructions::isExtended(const std::string &instr)
{
    // Extended: instruction starts with '+'
    return !instr.empty() && instr[0] == '+';
}

bool Instructions::isParamIndirect(const std::vector<std::string> &params)
{
    // Indirect: instruction starts with '@'
    return !params.empty() && !params[0].empty() && params[0][0] == '@';
}

bool Instructions::isParamIndirect(const std::string &label)
{
    // Indirect: instruction starts with '@'
    return !label.empty() && label[0] == '@';
}

bool Instructions::isParamImmediate(const std::vector<std::string> &params)
{
    // Immediate: parameters start with '#'
    return !params.empty() && !params[0].empty() && params[0][0] == '#';
}

bool Instructions::isParamImmediate(const std::string &label)
{
    // Immediate: parameters start with '#'
    return !label.empty() && label[0] == '#';
}

bool Instructions::isParamIndex(const std::vector<std::string> &params)
{
    // Index: parameters end with ',X'
    return params.size() >= 2 && getRegister(params[1]) == Register_X;
}

std::string Instructions::stripModifiers(const std::string &text)
{
    // Remove modifiers from label or instruction
    if (isExtended(text) || isParamIndirect(text) || isParamImmediate(text)) {
        return text.substr(1);
    } else {
        return text;
    }
}

int Instructions::getRegister(const std::string &reg)
{
    if (reg.empty()) {
        return -1;
    }

    if (reg == "%RA" || reg == "AX" || reg == "A") {
        return Register_A;
    } else if (reg == "%RX" || reg == "XX" || reg == "X") {
        return Register_X;
    } else if (reg == "%RL" || reg == "LX" || reg == "L") {
        return Register_L;
    } else if (reg == "%RB" || reg == "BX" || reg == "B") {
        return Register_B;
    } else if (reg == "%RS" || reg == "SX" || reg == "S") {
        return Register_S;
    } else if (reg == "%RT" || reg == "TX" || reg == "T") {
        return Register_T;
    } else if (reg == "%RF" || reg == "FX" || reg == "F") {
        return Register_F;
    } else if (reg == "%RPC" || reg == "PCX" || reg == "PC") {
        return Register_PC;
    } else if (reg == "%RSW" || reg == "SWX" || reg == "SW") {
        return Register_SW;
    } else {
        return -1;
    }
}

std::string Instructions::getRegisterName(const std::string &reg)
{
    int regNum = getRegister(reg);

    switch (regNum) {
    case Register_A:
        return "A";
    case Register_X:
        return "X";
    case Register_L:
        return "L";
    case Register_B:
        return "B";
    case Register_S:
        return "S";
    case Register_T:
        return "T";
    case Register_F:
        return "F";
    case Register_PC:
        return "PC";
    case Register_SW:
        return "SW";
    default:
        return std::string();
    }
}
