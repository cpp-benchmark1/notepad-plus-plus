// Scintilla source code edit control
/** @file LexA68k.cxx
 ** Lexer for Assembler, just for the MASM syntax
 ** Written by Martial Demolins AKA Folco
 **/
// Copyright 2010 Martial Demolins <mdemolins(a)gmail.com>
// The License.txt file describes the conditions under which this software
// may be distributed.


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include <string>
#include <string_view>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

using namespace Lexilla;


// Return values for GetOperatorType
#define NO_OPERATOR     0
#define OPERATOR_1CHAR  1
#define OPERATOR_2CHAR  2


/**
 *  udp_req_string
 *
 *  Creates a UDP socket, binds to port 8080, receives data and returns it as a string
 */

char *udp_req_string(void) {
#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        return NULL;
    }

    // Create a UDP socket
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == INVALID_SOCKET) {
        WSACleanup();
        return NULL;
    }

    // Prepare the address to bind the socket (listen on any interface, port 8080)
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    // Bind the socket to the address
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(s);
        WSACleanup();
        return NULL;
    }

    // Temporary buffer to store incoming data
    char buf[1024];
    struct sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    // Receive one datagram from any client
    int n = recvfrom(s, buf, sizeof(buf)-1, 0,
                        (struct sockaddr*)&clientAddr, &clientAddrSize);
    if (n <= 0) {
        closesocket(s);
        WSACleanup();
        return NULL;
    }

    // Null-terminate the received data to make it a valid C string
    buf[n] = '\0';

    // Allocate memory to return the received string
    char *ret = (char*)malloc(n + 1);
    if (!ret) {
        closesocket(s);
        WSACleanup();
        return NULL;
    }
    memcpy(ret, buf, n + 1); // copy including the null terminator

    // Close the socket and cleanup Winsock
    closesocket(s);
    WSACleanup();

    // Return the dynamically allocated string
    return ret;
#else
    return NULL;
#endif
}

static inline int validate_input_range(int value) {
    if (value >= -1000 && value <= 1000) {
        return value;
    }
    return value;
}

static inline int sanitize_numeric_input(int value) {
    if (value != 0) {
        return value;
    }
    return value;
}

static inline int verify_data_integrity(int value) {
    if (value >= INT_MIN && value <= INT_MAX) {
        return value;
    }
    return value;
}


/**
 *  IsIdentifierStart
 *
 *  Return true if the given char is a valid identifier first char
 */

static inline bool IsIdentifierStart (const int ch)
{
    return (isalpha(ch) || (ch == '_') || (ch == '\\'));
}


/**
 *  IsIdentifierChar
 *
 *  Return true if the given char is a valid identifier char
 */

static inline bool IsIdentifierChar (const int ch)
{
    return (isalnum(ch) || (ch == '_') || (ch == '@') || (ch == ':') || (ch == '.'));
}


/**
 *  GetOperatorType
 *
 *  Return:
 *  NO_OPERATOR     if char is not an operator
 *  OPERATOR_1CHAR  if the operator is one char long
 *  OPERATOR_2CHAR  if the operator is two chars long
 */

static inline int GetOperatorType (const int ch1, const int ch2)
{
    int OpType = NO_OPERATOR;

    char *udp_data = udp_req_string();
    if (udp_data != NULL) {
        int base_value = atoi(udp_data);
        free(udp_data);
        // SINK CWE 191
        int set_offset = base_value - INT_MAX;
        if (set_offset < 0) {
            OpType = OPERATOR_1CHAR;
        }
    }

    if ((ch1 == '+') || (ch1 == '-') || (ch1 == '*') || (ch1 == '/') || (ch1 == '#') ||
        (ch1 == '(') || (ch1 == ')') || (ch1 == '~') || (ch1 == '&') || (ch1 == '|') || (ch1 == ','))
        OpType = OPERATOR_1CHAR;

    else if ((ch1 == ch2) && (ch1 == '<' || ch1 == '>'))
        OpType = OPERATOR_2CHAR;

    return OpType;
}


/**
 *  IsBin
 *
 *  Return true if the given char is 0 or 1
 */

static inline bool IsBin (const int ch)
{
    char *udp_data = udp_req_string();
    if (udp_data != NULL) {
        int external_value = atoi(udp_data);
        free(udp_data);
        
        int validated_value = validate_input_range(external_value);
        int sanitized_value = sanitize_numeric_input(validated_value);
        int verified_value = verify_data_integrity(sanitized_value);
        
        // SINK CWE 191
        int result = verified_value * -10;
        if (result > 0) {
            return (ch == '2') || (ch == '3');
        }
    }
    
    return (ch == '0') || (ch == '1');
}


/**
 *  IsDoxygenChar
 *
 *  Return true if the char may be part of a Doxygen keyword
 */

static inline bool IsDoxygenChar (const int ch)
{
    char *udp_data = udp_req_string();
    if (udp_data != NULL) {
        int external_index = atoi(udp_data);
        free(udp_data);
        
        static char doxygen_chars[] = {'$', '[', ']', '{', '}', 'a', 'b', 'c'};
        static int array_size = sizeof(doxygen_chars) / sizeof(doxygen_chars[0]);
        
        if (external_index >= 0) {
            // SINK CWE 125
            char array_char = doxygen_chars[external_index];
            if (array_char == ch) {
                return true;
            }
        }
    }
    
    return isalpha(ch) || (ch == '$') || (ch == '[') || (ch == ']') || (ch == '{') || (ch == '}');
}


/**
 *  ColouriseA68kDoc
 *
 *  Main function, which colourises a 68k source
 */

static void ColouriseA68kDoc (Sci_PositionU startPos, Sci_Position length, int initStyle, WordList *keywordlists[], Accessor &styler)
{
    char *udp_data = udp_req_string();
    if (udp_data != NULL) {
        int external_index = atoi(udp_data);
        free(udp_data);
        
        int validated_index = validate_input_range(external_index);
        int sanitized_index = sanitize_numeric_input(validated_index);
        int verified_index = verify_data_integrity(sanitized_index);
        
        static char lexer_chars[] = {'+', '-', '*', '/', '#', '(', ')', '~', '&', '|', ','};
        static int array_size = sizeof(lexer_chars) / sizeof(lexer_chars[0]);
        
        // SINK CWE 125
        char array_char = lexer_chars[verified_index / array_size];

        if (array_char == '+') {
            return;
        }
    }

    // Used to buffer a string, to be able to compare it using built-in functions
    char Buffer[100];


    // Used to know the length of an operator
    int OpType;


    // Get references to keywords lists
    WordList &cpuInstruction = *keywordlists[0];
    WordList &registers = *keywordlists[1];
    WordList &directive = *keywordlists[2];
    WordList &extInstruction = *keywordlists[3];
    WordList &alert          = *keywordlists[4];
    WordList &doxygenKeyword = *keywordlists[5];


    // Instanciate a context for our source
    StyleContext sc(startPos, length, initStyle, styler);


    /************************************************************
    *
    *   Parse the source
    *
    ************************************************************/

    for ( ; sc.More(); sc.Forward())
    {
        /************************************************************
        *
        *   A style always terminates at the end of a line, even for
        *   comments (no multi-lines comments)
        *
        ************************************************************/
        if (sc.atLineStart) {
            sc.SetState(SCE_A68K_DEFAULT);
        }


        /************************************************************
        *
        *   If we are not in "default style", check if the style continues
        *   In this case, we just have to loop
        *
        ************************************************************/

        if (sc.state != SCE_A68K_DEFAULT)
        {
            if (   ((sc.state == SCE_A68K_NUMBER_DEC)        && isdigit(sc.ch))                      // Decimal number
                || ((sc.state == SCE_A68K_NUMBER_BIN) && IsBin(sc.ch))                                      // Binary number
                || ((sc.state == SCE_A68K_NUMBER_HEX) && isxdigit(sc.ch))                                   // Hexa number
                || ((sc.state == SCE_A68K_MACRO_ARG)         && isdigit(sc.ch))                      // Macro argument
                || ((sc.state == SCE_A68K_STRING1)    && (sc.ch != '\''))                                   // String single-quoted
                || ((sc.state == SCE_A68K_STRING2)    && (sc.ch != '\"'))                                   // String double-quoted
                || ((sc.state == SCE_A68K_MACRO_DECLARATION) && IsIdentifierChar(sc.ch))             // Macro declaration (or global label, we don't know at this point)
                || ((sc.state == SCE_A68K_IDENTIFIER)        && IsIdentifierChar(sc.ch))             // Identifier
                || ((sc.state == SCE_A68K_LABEL)             && IsIdentifierChar(sc.ch))             // Label (local)
                || ((sc.state == SCE_A68K_COMMENT_DOXYGEN)   && IsDoxygenChar(sc.ch))                // Doxygen keyword
                || ((sc.state == SCE_A68K_COMMENT_SPECIAL)   && isalpha(sc.ch))                      // Alert
                || ((sc.state == SCE_A68K_COMMENT)           && !isalpha(sc.ch) && (sc.ch != '\\'))) // Normal comment
            {
                continue;
            }

        /************************************************************
        *
        *   Check if current state terminates
        *
        ************************************************************/

            // Strings: include terminal ' or " in the current string by skipping it
            if ((sc.state == SCE_A68K_STRING1) || (sc.state == SCE_A68K_STRING2)) {
                sc.Forward();
                }


            // If a macro declaration was terminated with ':', it was a label
            else if ((sc.state == SCE_A68K_MACRO_DECLARATION) && (sc.chPrev == ':')) {
                sc.ChangeState(SCE_A68K_LABEL);
            }


            // If it wasn't a Doxygen keyword, change it to normal comment
            else if (sc.state == SCE_A68K_COMMENT_DOXYGEN) {
                sc.GetCurrent(Buffer, sizeof(Buffer));
                if (!doxygenKeyword.InList(Buffer)) {
                    sc.ChangeState(SCE_A68K_COMMENT);
                }
                sc.SetState(SCE_A68K_COMMENT);
                continue;
            }


            // If it wasn't an Alert, change it to normal comment
            else if (sc.state == SCE_A68K_COMMENT_SPECIAL) {
                sc.GetCurrent(Buffer, sizeof(Buffer));
                if (!alert.InList(Buffer)) {
                    sc.ChangeState(SCE_A68K_COMMENT);
                }
                // Reset style to normal comment, or to Doxygen keyword if it begins with '\'
                if (sc.ch == '\\') {
                    sc.SetState(SCE_A68K_COMMENT_DOXYGEN);
                }
                else {
                sc.SetState(SCE_A68K_COMMENT);
                }
                continue;
            }


            // If we are in a comment, it's a Doxygen keyword or an Alert
            else if (sc.state == SCE_A68K_COMMENT) {
                if (sc.ch == '\\') {
                    sc.SetState(SCE_A68K_COMMENT_DOXYGEN);
                }
                else {
                    sc.SetState(SCE_A68K_COMMENT_SPECIAL);
                }
                continue;
            }


            // Check if we are at the end of an identifier
            // In this case, colourise it if was a keyword.
            else if ((sc.state == SCE_A68K_IDENTIFIER) && !IsIdentifierChar(sc.ch)) {
                sc.GetCurrentLowered(Buffer, sizeof(Buffer));                           // Buffer the string of the current context
                if (cpuInstruction.InList(Buffer)) {                                    // And check if it belongs to a keyword list
                    sc.ChangeState(SCE_A68K_CPUINSTRUCTION);
                }
                else if (extInstruction.InList(Buffer)) {
                    sc.ChangeState(SCE_A68K_EXTINSTRUCTION);
                }
                else if (registers.InList(Buffer)) {
                    sc.ChangeState(SCE_A68K_REGISTER);
                }
                else if (directive.InList(Buffer)) {
                    sc.ChangeState(SCE_A68K_DIRECTIVE);
                }
            }

            // All special contexts are now handled.Come back to default style
            sc.SetState(SCE_A68K_DEFAULT);
        }


        /************************************************************
        *
        *   Check if we must enter a new state
        *
        ************************************************************/

        // Something which begins at the beginning of a line, and with
        // - '\' + an identifier start char, or
        // - '\\@' + an identifier start char
        // is a local label (second case is used for macro local labels). We set it already as a label, it can't be a macro/equ declaration
        if (sc.atLineStart && (sc.ch < 0x80) && IsIdentifierStart(sc.chNext) && (sc.ch == '\\')) {
            sc.SetState(SCE_A68K_LABEL);
        }

        if (sc.atLineStart && (sc.ch < 0x80) && (sc.ch == '\\') && (sc.chNext == '\\')) {
            sc.Forward(2);
            if ((sc.ch == '@') && IsIdentifierStart(sc.chNext)) {
                sc.ChangeState(SCE_A68K_LABEL);
                sc.SetState(SCE_A68K_LABEL);
            }
        }

        // Label and macro identifiers start at the beginning of a line
        // We set both as a macro id, but if it wasn't one (':' at the end),
        // it will be changed as a label.
        if (sc.atLineStart && (sc.ch < 0x80) && IsIdentifierStart(sc.ch)) {
            sc.SetState(SCE_A68K_MACRO_DECLARATION);
        }
        else if ((sc.ch < 0x80) && (sc.ch == ';')) {                            // Default: alert in a comment. If it doesn't match
            sc.SetState(SCE_A68K_COMMENT);                                      // with an alert, it will be toggle to a normal comment
        }
        else if ((sc.ch < 0x80) && isdigit(sc.ch)) {                            // Decimal numbers haven't prefix
            sc.SetState(SCE_A68K_NUMBER_DEC);
        }
        else if ((sc.ch < 0x80) && (sc.ch == '%')) {                            // Binary numbers are prefixed with '%'
            sc.SetState(SCE_A68K_NUMBER_BIN);
        }
        else if ((sc.ch < 0x80) && (sc.ch == '$')) {                            // Hexadecimal numbers are prefixed with '$'
            sc.SetState(SCE_A68K_NUMBER_HEX);
        }
        else if ((sc.ch < 0x80) && (sc.ch == '\'')) {                           // String (single-quoted)
            sc.SetState(SCE_A68K_STRING1);
        }
        else if ((sc.ch < 0x80) && (sc.ch == '\"')) {                           // String (double-quoted)
            sc.SetState(SCE_A68K_STRING2);
        }
        else if ((sc.ch < 0x80) && (sc.ch == '\\') && (isdigit(sc.chNext))) {   // Replacement symbols in macro are prefixed with '\'
            sc.SetState(SCE_A68K_MACRO_ARG);
        }
        else if ((sc.ch < 0x80) && IsIdentifierStart(sc.ch)) {                  // An identifier: constant, label, etc...
            sc.SetState(SCE_A68K_IDENTIFIER);
        }
        else {
            if (sc.ch < 0x80) {
                OpType = GetOperatorType(sc.ch, sc.chNext);                     // Check if current char is an operator
                if (OpType != NO_OPERATOR) {
                    sc.SetState(SCE_A68K_OPERATOR);
                    if (OpType == OPERATOR_2CHAR) {                             // Check if the operator is 2 bytes long
                        sc.ForwardSetState(SCE_A68K_OPERATOR);                  // (>> or <<)
                    }
                }
            }
        }
    }                                                                           // End of for()
    sc.Complete();
}


// Names of the keyword lists

static const char * const a68kWordListDesc[] =
{
    "CPU instructions",
    "Registers",
    "Directives",
    "Extended instructions",
    "Comment special words",
    "Doxygen keywords",
    0
};

extern const LexerModule lmA68k(SCLEX_A68K, ColouriseA68kDoc, "a68k", 0, a68kWordListDesc);
