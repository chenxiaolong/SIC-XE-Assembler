#include "assembler.h"

#include <cassert>
#include <cstdio>

#include <stdarg.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include <algorithm>


static bool strtolWrap(const char *str, int base, int *out)
{
    char *ptr;
    *out = strtoul(str, &ptr, base);
    if (*ptr != '\0') {
        return false;
    }

    return true;
}

static bool strtoulWrap(const char *str, int base, unsigned int *out)
{
    char *ptr;
    *out = strtoul(str, &ptr, base);
    if (*ptr != '\0') {
        return false;
    }

    return true;
}

Assembler::Assembler()
{
}

Assembler::~Assembler()
{
    for (auto *asmLine : m_lines) {
        delete asmLine;
    }
}

__attribute__((format(printf, 3, 4)))
void Assembler::error(ASMLine *asmLine, const char *fmt, ...)
{
    // Filename and line number
    if (asmLine) {
        std::fprintf(stderr, "%s:%u: ", m_path.c_str(), asmLine->lineNumber);
    }

    std::fprintf(stderr, "error: ");

    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
    std::fprintf(stderr, "\n");

    if (asmLine) {
        std::fprintf(stderr, "    %s\n", asmLine->line.c_str());
    }
}

bool Assembler::assembleFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    m_path = path;

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && *line.crbegin() == '\r') {
            line.erase(line.end() - 1);
        }
        lines.push_back(line);
    }

    file.close();

    if (!pass1(lines)) {
        return false;
    }

    if (!pass2()) {
        return false;
    }

    std::string lstPath;
    std::string objPath;
    if (path.size() >= 4 && path.compare(path.size() - 4, 4, ".asm") == 0) {
        lstPath = path.substr(0, path.size() - 4) + ".lst";
        objPath = path.substr(0, path.size() - 4) + ".obj";
    } else {
        lstPath = path + ".lst";
        objPath = path + ".obj";
    }

    if (!writeOutput(lstPath, objPath)) {
        return false;
    }

    return true;
}

bool Assembler::pass1(const std::vector<std::string> &lines)
{
    m_loc = 0;
    m_name.clear();

    int lineNumber = 0;

    for (auto const &line : lines) {
        ++lineNumber;

        std::vector<std::string> tokens;
        splitString(&tokens, line, ' ');

        if (tokens.empty()) {
            // Skip empty lines
            continue;
        }

        if (tokens[0][0] == '.') {
            // Skip comment lines
            continue;
        }

        ASMLine *asmLine = new ASMLine();
        m_lines.push_back(asmLine);
        asmLine->line = line;
        asmLine->lineNumber = lineNumber;

        std::string label;
        std::string instr;
        int paramIndex;

        if (m_instrs.isValid(tokens[0])) {
            // First token is instruction and there's no label
            instr = tokens[0];
            paramIndex = 1;
        } else if (tokens.size() >= 2 && m_instrs.isValid(tokens[1])) {
            // First token is label, second token is instruction
            label = tokens[0];
            instr = tokens[1];
            paramIndex = 2;
        } else {
            error(asmLine, "Invalid instruction %s", tokens[0].c_str());
            return false;
        }

        if (Instructions::getRegister(label) >= 0) {
            error(asmLine, "Label cannot be the name of a register: %s",
                  label.c_str());
            return false;
        }

        // Treat remaining tokens in the line as parameters and split them at each
        // comma if necessary
        for (unsigned int i = paramIndex; i < tokens.size(); ++i) {
            std::vector<std::string> commaSplit;
            splitString(&commaSplit, tokens[i], ',');
            if (!tokens[i].empty()) {
                asmLine->params.insert(asmLine->params.end(),
                                       commaSplit.begin(), commaSplit.end());
            }
        }

        // Replace ie. BUFFER[%RX] with BUFFER,X
        convertIndexing(&asmLine->params);

        if (Instructions::stripModifiers(instr) == Instructions::Additional_MOV) {
            std::string newInstr;
            std::vector<std::string> newParams;

            bool ret = convertMovToSicXE(asmLine->params,
                                        Instructions::isExtended(instr),
                                        &newInstr, &newParams);
            if (!ret) {
                error(asmLine, "%s", m_error.c_str());
                return false;
            }

            instr = newInstr;
            asmLine->params.swap(newParams);
        } else if (Instructions::stripModifiers(instr) == Instructions::Additional_LD
                || Instructions::stripModifiers(instr) == Instructions::Additional_ST) {
            std::string newInstr;
            std::vector<std::string> newParams;

            bool ret = convertLdStToSicXE(Instructions::stripModifiers(instr),
                                          asmLine->params,
                                          Instructions::isExtended(instr),
                                          &newInstr, &newParams);
            if (!ret) {
                error(asmLine, "%s", m_error.c_str());
                return false;
            }

            instr = newInstr;
            asmLine->params.swap(newParams);
        }

        asmLine->label = label;
        asmLine->instr = instr;

        // Calculate locations

        if (instr == Instructions::Directive_START) {
            if (asmLine->params.size() != 1) {
                error(asmLine, "START accepts 1 argument");
                return false;
            }

            // Set initial location to the parameter (in hex)
            if (!strtoulWrap(asmLine->params[0].c_str(), 16, &m_start)) {
                error(asmLine, "Invalid hex address: %s",
                      asmLine->params[0].c_str());
                return false;
            }

            m_loc = m_start;
            m_name = asmLine->label;
        } else if (instr == Instructions::Directive_END) {
            // END is unimportant
        } else if (instr == Instructions::Directive_BASE
                || instr == Instructions::Directive_NOBASE) {
            // Base directives do not affect the location
        } else if (m_instrs.isSicXE(instr)) {
            auto *info = m_instrs[instr];

            // Save location and increment appropriately
            asmLine->location = m_loc;

            switch (info->length) {
            case Instructions::Length::One:
                m_loc += 1;
                break;
            case Instructions::Length::Two:
                m_loc += 2;
                break;
            case Instructions::Length::ThreeOrFour:
                if (m_instrs.isExtended(instr)) {
                    m_loc += 4;
                } else {
                    m_loc += 3;
                }
                break;
            }
        } else if (instr == Instructions::Variable_WORD) {
            asmLine->location = m_loc;

            // A word is 3 bytes
            m_loc += 3;
        } else if (instr == Instructions::Variable_RESW) {
            if (asmLine->params.size() != 1) {
                error(asmLine, "RESW accepts 1 argument");
                return false;
            }

            asmLine->location = m_loc;

            // An array of words is: 3 bytes * length
            unsigned int length;
            if (!strtoulWrap(asmLine->params[0].c_str(), 10, &length)) {
                error(asmLine, "Invalid length: %s",
                      asmLine->params[0].c_str());
                return false;
            }

            m_loc += 3 * length;
        } else if (instr == Instructions::Variable_RESB) {
            if (asmLine->params.size() != 1) {
                error(asmLine, "RESB accepts 1 argument");
                return false;
            }

            asmLine->location = m_loc;

            // An array of bytes is: 1 byte * length
            unsigned int length;
            if (!strtoulWrap(asmLine->params[0].c_str(), 10, &length)) {
                error(asmLine, "Invalid length: %s",
                      asmLine->params[0].c_str());
                return false;
            }

            m_loc += length;
        } else if (instr == Instructions::Variable_BYTE) {
            if (asmLine->params.size() != 1) {
                error(asmLine, "BYTE accepts 1 argument");
                return false;
            }

            asmLine->location = m_loc;

            std::string quoted = getQuoted(asmLine->params[0]);
            if (quoted.empty()) {
                error(asmLine, "No bytes found in BYTE variable: %s",
                      asmLine->params[0].c_str());
                return false;
            }

            m_loc += quoted.length();
        } else {
            // Programmer's error
            assert(false);
        }

        asmLine->locationNext = m_loc;
    }

    if (m_lines.empty()) {
        error(nullptr, "Empty assembly file");
        return false;
    }

    return true;
}

bool Assembler::pass2()
{
    for (unsigned int index = 0; index < m_lines.size(); ++index) {
        ASMLine *asmLine = m_lines[index];

        if (m_instrs.isSicXE(asmLine->instr)) {
            auto *instr = m_instrs[asmLine->instr];

            std::size_t length;
            switch (instr->type) {
            case Instructions::Type::ZeroOp:
                length = 0;
                break;
            case Instructions::Type::OneOp:
                length = 1;
                break;
            case Instructions::Type::TwoOp:
                length = 2;
                break;
            default:
                // Programmer's error
                assert(false);
            }

            std::size_t paramSize = asmLine->params.size();

            // Instruction with non-register parameters and index mode will
            // have an extra "X" parameter
            if (instr->type == Instructions::Type::OneOp
                    && Instructions::isParamIndex(asmLine->params)) {
                --paramSize;
            }

            if (paramSize != length) {
                error(asmLine, "%s accepts %zu arguments",
                      Instructions::stripModifiers(asmLine->instr).c_str(),
                      length);
                return false;
            }

            if (instr->length == Instructions::Length::One) {
                // One byte instructions
                asmLine->objectCode = getObjCode1Byte(instr);
            } else if (instr->length == Instructions::Length::Two) {
                switch (instr->type) {
                case Instructions::Type::OneOp:
                    // Two byte, one operand instructions
                    asmLine->objectCode = getObjCode2Bytes(
                            instr, asmLine->params[0], std::string());
                    break;
                case Instructions::Type::TwoOp:
                    // Two byte, two operand instructions
                    asmLine->objectCode = getObjCode2Bytes(
                            instr, asmLine->params[0], asmLine->params[1]);
                    break;
                case Instructions::Type::ZeroOp:
                    // Programmer error
                    assert(false);
                }
            } else if (instr->length == Instructions::Length::ThreeOrFour) {
                // Three or four byte, zero or two operand instructions
                asmLine->objectCode = getObjCode3Or4Bytes(
                        instr,                  // Instruction info
                        asmLine->instr,         // Instruction
                        asmLine->params,        // Instruction parameters
                        asmLine->locationNext,  // Program counter value
                        m_base);                // Base register value
            } else {
                // Programmer's error
                assert(false);
            }

            if (asmLine->objectCode < 0) {
                error(asmLine, "Failed to generate object code: %s",
                      m_error.c_str());
                return false;
            }
        } else if (asmLine->instr == Instructions::Directive_BASE) {
            if (asmLine->params.size() != 1) {
                error(asmLine, "BASE accepts 1 parameter");
                return false;
            }

            // Set base value appropriately
            if (!findLabelAddr(asmLine->params[0], &m_base)) {
                error(asmLine, "Label not found: %s",
                      asmLine->params[0].c_str());
                return false;
            }
        } else if (asmLine->instr == Instructions::Directive_NOBASE) {
            // Disable use of base-relative addressing
            m_base = -1;
        }
    }

    return true;
}

bool Assembler::writeOutput(const std::string &listingFile,
                            const std::string &objectFile)
{
    // cstdio is better for writing hex information than iostream
    std::FILE *lst = std::fopen(listingFile.c_str(), "wb");
    if (lst == nullptr) {
        return false;
    }

    std::FILE *obj = std::fopen(objectFile.c_str(), "wb");
    if (obj == nullptr) {
        std::fclose(lst);
        return false;
    }

    // Write object code header
    // Col 1:     H
    // Col 2-7:   Program name
    // Col 8-13:  Starting address
    // Col 14-19: Length of program
    std::fprintf(obj, "H%-6s%06X%06x\n", m_name.c_str(), m_start, m_loc);

    //int objLinePos = 0;
    unsigned int lineIndex = 0;

    // Current 60 char OP code
    std::string curObjCode;

    while (lineIndex < m_lines.size()) {
        int startingAddr = m_lines[lineIndex]->location;

        while (curObjCode.size() <= 60 && lineIndex < m_lines.size()) {
            std::string objCode = getObjCodeStr(m_lines[lineIndex]);
            if (curObjCode.size() + objCode.size() > 60) {
                break;
            }

            curObjCode += objCode;
            lineIndex++;
        }

        // Write text header
        // Col 1:     T
        // Col 2-7:   Starting address for object code
        // Col 8-9:   Length of object code
        // Col 10-69: Object code in hex
        std::fprintf(obj, "T%06X%02lX%s\n", startingAddr,
                     curObjCode.size() / 2, curObjCode.c_str());

        curObjCode.clear();
    }

    // End header
    // Col 1:   E
    // Col 2-7: Address of first executable instruction
    std::fprintf(obj, "E%06X\n", m_start);


    // Write listing file
    std::size_t maxLength = 0;
    for (ASMLine *line : m_lines) {
        if (line->line.size() > maxLength) {
            maxLength = line->line.size();
        }
    }

    for (ASMLine *asmLine : m_lines) {
        // Print location
        if (asmLine->instr == Instructions::Directive_START
                || m_instrs.isSicXE(asmLine->instr)
                || m_instrs.isVariable(asmLine->instr)) {
            std::fprintf(lst, "%04X    ", asmLine->location);
        } else {
            std::fprintf(lst, "        ");
        }

        // Print original code
        std::fprintf(lst, "%s", asmLine->line.c_str());

        std::string objCode = getObjCodeStr(asmLine);
        if (!objCode.empty()) {
            std::fprintf(lst, "%s", std::string(
                    maxLength - asmLine->line.size(), ' ').c_str());
            std::fprintf(lst, "    %s\n", objCode.c_str());
        } else {
            std::fprintf(lst, "\n");
        }
    }

    std::fclose(lst);
    std::fclose(obj);

    return true;
}

/* Split string using delimiter and keep only non-empty tokens */
void Assembler::splitString(std::vector<std::string> *out, const std::string &in, char delim)
{
    out->clear();

    std::istringstream ss(in);
    std::string temp;
    while (std::getline(ss, temp, delim)) {
        // Skip empty
        if (!temp.empty()) {
            out->push_back(temp);
        }
    }
}

/* Search table for the address of a label */
bool Assembler::findLabelAddr(const std::string &label, unsigned int *out)
{
    std::string labelOnly = Instructions::stripModifiers(label);

    for (ASMLine *line : m_lines) {
        if (line->label == labelOnly) {
            *out = line->location;
            return true;
        }
    }

    return false;
}

/* Convert the additional MOV instruction to the SIC/XE equivalent */
bool Assembler::convertMovToSicXE(const std::vector<std::string> &params,
                                  bool extended,
                                  std::string *instrOut,
                                  std::vector<std::string> *paramsOut)
{
    if (params.size() != 2) {
        m_error = "MOV accepts 2 parameters";
        return false;
    }

    bool param1IsReg = Instructions::getRegister(params[0]) >= 0;
    bool param2IsReg = Instructions::getRegister(params[1]) >= 0;

    paramsOut->clear();

    if (param1IsReg && param2IsReg) {
        // Use RMO to move R1 to R2
        *instrOut = m_instrs[Instructions::SicXE::RMO]->name;
        paramsOut->push_back(params[1]);
        paramsOut->push_back(params[0]);
    } else if (param1IsReg) {
        std::string regName = Instructions::getRegisterName(params[0]);
        std::string instr = "LD" + regName;

        if (!m_instrs.isSicXE(instr)) {
            m_error = "Failed to convert MOV statement: "
                    + instr + " is invalid";
            return false;
        }

        *instrOut = instr;
        paramsOut->push_back(params[1]);
    } else {
        std::string regName = Instructions::getRegisterName(params[1]);
        std::string instr = "ST" + regName;

        if (!m_instrs.isSicXE(instr)) {
            m_error = "Failed to convert MOV statement: "
                    + instr + " is invalid";
            return false;
        }

        *instrOut = instr;
        paramsOut->push_back(params[0]);
    }

    if (extended) {
        // If MOV was extended, then the new instruction should be extended
        instrOut->insert(instrOut->begin(), '+');
    }

    return true;
}

bool Assembler::convertLdStToSicXE(const std::string &instr,
                                   const std::vector<std::string> &params,
                                   bool extended,
                                   std::string *instrOut,
                                   std::vector<std::string> *paramsOut)
{
    if (params.size() != 2) {
        m_error = instr + " accepts 2 parameters";
        return false;
    }

    bool param1IsReg = Instructions::getRegister(params[0]) >= 0;
    bool param2IsReg = Instructions::getRegister(params[1]) >= 0;

    paramsOut->clear();

    if (param1IsReg && param2IsReg) {
        // Use RMO to move R1 to R2
        *instrOut = m_instrs[Instructions::SicXE::RMO]->name;
        paramsOut->push_back(params[1]);
        paramsOut->push_back(params[0]);
    } else {
        if (!param2IsReg && instr == Instructions::Additional_ST) {
            m_error = "Failed to convert ST statement: ";
            m_error += "Second parameter ";
            m_error += params[1];
            m_error += " is not a register";
            return false;
        }
        if (!param1IsReg && instr == Instructions::Additional_LD) {
            m_error = "Failed to convert LD statement: ";
            m_error += "First parameter ";
            m_error += params[0];
            m_error += " is not a register";
            return false;
        }

        std::string reg;
        std::string value;

        if (instr == Instructions::Additional_ST) {
            reg = params[1];
            value = params[0];
        } else if (instr == Instructions::Additional_LD) {
            reg = params[0];
            value = params[1];
        }

        std::string regName = Instructions::getRegisterName(reg);
        std::string newInstr = instr + regName;

        if (!m_instrs.isSicXE(newInstr)) {
            m_error = "Failed to convert ";
            m_error += instr;
            m_error += " statement: ";
            m_error += newInstr;
            m_error += " is invalid";
            return false;
        }

        *instrOut = newInstr;
        paramsOut->push_back(value);
    }

    if (extended) {
        // If MOV was extended, then the new instruction should be extended
        instrOut->insert(instrOut->begin(), '+');
    }

    return true;
}

/* Convert parameters in the form BUFFER[%RX] to the form BUFFER,X */
void Assembler::convertIndexing(std::vector<std::string> *params)
{
    for (auto it = params->begin(); it != params->end(); ++it) {
        std::size_t leftBracket = it->find('[');
        std::size_t rightBracket = it->find(']');

        if (leftBracket != std::string::npos
                && rightBracket != std::string::npos
                && leftBracket < rightBracket) {
            // Get parameter before the left bracket
            std::string param = it->substr(0, leftBracket);

            // No need to parse inside of brackets, the only valid register for
            // indexing is the X register.
            *it = param;
            it = params->insert(++it, "X");
        }
    }
}

/* Get quoted portion for a BYTE variable. This returns the string inside quotes
 * if the SIC/XE parameter is in the form C'ABC' or the hex bytes if the
 * paramter is in the form X'7F7F7F'. */
std::string Assembler::getQuoted(const std::string &str)
{
    std::size_t leftQuote = str.find('\'');
    std::size_t rightQuote = str.rfind('\'');

    if (leftQuote != std::string::npos && rightQuote != std::string::npos) {
        std::string quoted = str.substr(leftQuote + 1, rightQuote - leftQuote - 1);

        if (!str.empty() && str[0] == 'C') {
            // Character bytes
            return quoted;
        } else if (!str.empty() && str[0] == 'X') {
            // Hex bytes
            std::string temp;
            for (unsigned int i = 0; (i + 1) < quoted.size(); i += 2) {
                unsigned char c = 16 * hexCharToInt(quoted[i])
                        + hexCharToInt(quoted[i + 1]);
                temp += c;
            }
            return temp;
        }
    }

    return std::string();
}

/* Convert single hex digit to integer */
int Assembler::hexCharToInt(unsigned char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else {
        return -1;
    }
}

/* Return object code as a hex string */
std::string Assembler::getObjCodeStr(ASMLine *asmLine)
{
    std::vector<char> objCode;

    if (m_instrs.isSicXE(asmLine->instr)) {
        auto *info = m_instrs[asmLine->instr];

        switch (info->length) {
        case Instructions::Length::One:
            objCode.resize(2 + 1);
            std::sprintf(objCode.data(), "%02X", asmLine->objectCode);
            break;
        case Instructions::Length::Two:
            objCode.resize(4 + 1);
            std::sprintf(objCode.data(), "%04X", asmLine->objectCode);
            break;
        case Instructions::Length::ThreeOrFour:
            if (Instructions::isExtended(asmLine->instr)) {
                objCode.resize(8 + 1);
                std::sprintf(objCode.data(), "%08X", asmLine->objectCode);
            } else {
                objCode.resize(6 + 1);
                std::sprintf(objCode.data(), "%06X", asmLine->objectCode);
            }
            break;
        }
    } else if (asmLine->instr == Instructions::Variable_BYTE) {
        std::string quoted = getQuoted(asmLine->params[0]);
        objCode.resize(2 * quoted.size() + 1);
        for (unsigned int i = 0; i < quoted.size(); ++i) {
            unsigned char c = quoted[i];
            std::sprintf(&objCode[2 * i], "%02X", c);
        }
    } else {
        objCode.push_back('\0');
    }

    return std::string(objCode.data());
}

/* Calculate object code for 1 byte instructions */
int Assembler::getObjCode1Byte(const Instructions::InstrInfo *info)
{
    return info->opcode;
}

/* Calculate object code for 2 byte instructions */
int Assembler::getObjCode2Bytes(const Instructions::InstrInfo *info,
                                const std::string &reg1, const std::string &reg2)
{
    int regId1, regId2;

    switch (info->type) {
    case Instructions::Type::OneOp:
        regId1 = Instructions::getRegister(reg1);
        regId2 = 0;

        if (regId1 < 0) {
            // Invalid register
            return -1;
        }

        break;

    case Instructions::Type::TwoOp:
        regId1 = Instructions::getRegister(reg1);
        regId2 = Instructions::getRegister(reg2);

        if (regId1 < 0 || regId2 < 0) {
            // Invalid registers
            return -1;
        }

        break;

    default:
        // Programmer's error
        assert(false);
    }

    int objCode = 0;
    objCode += (info->opcode << 8);
    objCode += ((regId1 & 0xF) << 4); // Operand 1 (maximum 4 bits)
    objCode += (regId2 & 0xF);        // Operand 2 (maximum 4 bits)
    return objCode;
}

/* Calculate object code for 3 byte and 4 byte instructions */
int Assembler::getObjCode3Or4Bytes(const Instructions::InstrInfo *info,
                                   const std::string &instr,
                                   const std::vector<std::string> &params,
                                   int prog, int base)
{
    bool indirect = Instructions::isParamIndirect(params);
    bool immediate = Instructions::isParamImmediate(params);
    bool index = Instructions::isParamIndex(params);
    bool extended = Instructions::isExtended(instr);

    bool useBase = false;
    bool useProg = false;

    int target = 0;
    int objCode;

    if (info->type == Instructions::Type::ZeroOp) {
        // If there are no operands, then the remaining bits are 0
        target = 0;
    } else if (info->type == Instructions::Type::TwoOp) {
        // There are no 3 or 4 byte instructions with two operands
        // Programmer error
        assert(false);
    } else {
        std::string targetStr = Instructions::stripModifiers(params[0]);

        if (strtolWrap(targetStr.c_str(), 10, &target)) {
            // If target is a number, use the constant directly
        } else {
            // Otherwise, it's a label
            unsigned int labelAddr;
            if (!findLabelAddr(targetStr, &labelAddr)) {
                m_error = "Label not found: ";
                m_error += targetStr;
                return -1;
            }

            if (extended) {
                // If extended, use absolute address
                target = labelAddr;
            } else {
                // Otherwise, calculate displacement
                int ret = getRelativeAddr(prog, base, labelAddr,
                                          &useProg, &useBase, &target);
                if (ret < 0) {
                    return -1;
                }
            }
        }
    }

    // For SIC/XE, the n-bit and i-bit should be 1 if they're both disabled
    if (!indirect && !immediate) {
        indirect = true;
        immediate = true;
    }

    objCode = 0;

    if (extended) {
        objCode += (info->opcode << 24);    // op code
        objCode += (indirect << 25);        // n
        objCode += (immediate << 24);       // i
        objCode += (index << 23);           // x
        objCode += (0 << 22);               // b (always 0)
        objCode += (0 << 21);               // p (always 0)
        objCode += (1 << 20);               // e (always 1)
        objCode += (target & 0xFFFFF);      // Target (20 bits)
    } else {
        objCode += (info->opcode << 16);    // op code
        objCode += (indirect << 17);        // n
        objCode += (immediate << 16);       // i
        objCode += (index << 15);           // x
        objCode += (useBase << 14);         // b
        objCode += (useProg << 13);         // p
        objCode += (extended << 12);        // e
        objCode += (target & 0xFFF);        // Target (12 bits)
    }

    return objCode;
}

/* Get address relative to base register or program counter register */
int Assembler::getRelativeAddr(int prog, int base, int target,
                               bool *useProg, bool *useBase, int *addr)
{
    int progDiff = target - prog;
    int baseDiff = target - base;

    int targetAddr;

    // Ensure that the distance is in range
    if (progDiff >= -2048 && progDiff <= 2047) {
        // Can use program counter relative
        *useProg = true;
        *useBase = false;
        targetAddr = progDiff;
    } else {
        *useProg = false;

        // Try base counter relative
        if (base < 0) {
            // Base register turned off
            m_error = "Base register not used and program counter out of range";
            return -1;
        }

        if (baseDiff >= 0 && baseDiff <= 4095) {
            // Can use program counter relative
            *useBase = true;
            targetAddr = baseDiff;
        } else {
            // Base out of range
            m_error = "Base and program counter displacement out of range";
            return -1;
        }
    }

    *addr = targetAddr;

    return 0;
}
