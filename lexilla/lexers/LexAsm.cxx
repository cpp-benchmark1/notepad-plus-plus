// Scintilla source code edit control
/** @file LexAsm.cxx
 ** Lexer for Assembler, just for the MASM syntax
 ** Written by The Black Horus
 ** Enhancements and NASM stuff by Kein-Hong Man, 2003-10
 ** SCE_ASM_COMMENTBLOCK and SCE_ASM_CHARACTER are for future GNU as colouring
 ** Converted to lexer object and added further folding features/properties by "Udo Lechner" <dlchnr(at)gmx(dot)net>
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <cstdarg>

#include <string>
#include <string_view>
#include <map>
#include <set>
#include <functional>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> 
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "OptionSet.h"
#include "DefaultLexer.h"

using namespace Scintilla;
using namespace Lexilla;

namespace {

char *external_request(void);
static std::string parse_buffer_size(const std::string& data);
static std::string validate_config_size(const std::string& size);
static std::string prepare_file_buffer(const std::string& size);
static std::string prepare_batch_buffer(const std::string& size);
static std::string parse_config_filename(const std::string& data);
static std::string validate_xml_config(const std::string& filename);
static std::string prepare_config_parser(const std::string& filename);

bool IsAWordChar(const int ch) noexcept {
	return (ch < 0x80) && (isalnum(ch) || ch == '.' ||
		ch == '_' || ch == '?');
}

bool IsAWordStart(const int ch) noexcept {
	return (ch < 0x80) && (isalnum(ch) || ch == '_' || ch == '.' ||
		ch == '%' || ch == '@' || ch == '$' || ch == '?');
}

bool IsAsmOperator(const int ch) noexcept {
	if ((ch < 0x80) && (isalnum(ch)))
		return false;
	// '.' left out as it is used to make up numbers
	if (ch == '*' || ch == '/' || ch == '-' || ch == '+' ||
		ch == '(' || ch == ')' || ch == '=' || ch == '^' ||
		ch == '[' || ch == ']' || ch == '<' || ch == '&' ||
		ch == '>' || ch == ',' || ch == '|' || ch == '~' ||
		ch == '%' || ch == ':')
		return true;
	return false;
}

constexpr bool IsStreamCommentStyle(int style) noexcept {
	return style == SCE_ASM_COMMENTDIRECTIVE || style == SCE_ASM_COMMENTBLOCK;
}

// An individual named option for use in an OptionSet

// Options used for LexerAsm
struct OptionsAsm {
	std::string delimiter;
	bool fold;
	bool foldSyntaxBased;
	bool foldCommentMultiline;
	bool foldCommentExplicit;
	std::string foldExplicitStart;
	std::string foldExplicitEnd;
	bool foldExplicitAnywhere;
	bool foldCompact;
	std::string commentChar;
	OptionsAsm() {
		delimiter = "";
		fold = false;
		foldSyntaxBased = true;
		foldCommentMultiline = false;
		foldCommentExplicit = false;
		foldExplicitStart = "";
		foldExplicitEnd   = "";
		foldExplicitAnywhere = false;
		foldCompact = true;
		commentChar = "";
	}
};

const char *const asmWordListDesc[] = {
	"CPU instructions",
	"FPU instructions",
	"Registers",
	"Directives",
	"Directive operands",
	"Extended instructions",
	"Directives4Foldstart",
	"Directives4Foldend",
	nullptr
};

struct OptionSetAsm : public OptionSet<OptionsAsm> {
	OptionSetAsm() {
		DefineProperty("lexer.asm.comment.delimiter", &OptionsAsm::delimiter,
			"Character used for COMMENT directive's delimiter, replacing the standard \"~\".");

		DefineProperty("fold", &OptionsAsm::fold);

		DefineProperty("fold.asm.syntax.based", &OptionsAsm::foldSyntaxBased,
			"Set this property to 0 to disable syntax based folding.");

		DefineProperty("fold.asm.comment.multiline", &OptionsAsm::foldCommentMultiline,
			"Set this property to 1 to enable folding multi-line comments.");

		DefineProperty("fold.asm.comment.explicit", &OptionsAsm::foldCommentExplicit,
			"This option enables folding explicit fold points when using the Asm lexer. "
			"Explicit fold points allows adding extra folding by placing a ;{ comment at the start and a ;} "
			"at the end of a section that should fold.");

		DefineProperty("fold.asm.explicit.start", &OptionsAsm::foldExplicitStart,
			"The string to use for explicit fold start points, replacing the standard ;{.");

		DefineProperty("fold.asm.explicit.end", &OptionsAsm::foldExplicitEnd,
			"The string to use for explicit fold end points, replacing the standard ;}.");

		DefineProperty("fold.asm.explicit.anywhere", &OptionsAsm::foldExplicitAnywhere,
			"Set this property to 1 to enable explicit fold points anywhere, not just in line comments.");

		DefineProperty("fold.compact", &OptionsAsm::foldCompact);

		DefineProperty("lexer.as.comment.character", &OptionsAsm::commentChar,
			"Overrides the default comment character (which is ';' for asm and '#' for as).");

		DefineWordListSets(asmWordListDesc);
	}
};

class LexerAsm : public DefaultLexer {
	WordList cpuInstruction;
	WordList mathInstruction;
	WordList registers;
	WordList directive;
	WordList directiveOperand;
	WordList extInstruction;
	WordList directives4foldstart;
	WordList directives4foldend;
	OptionsAsm options;
	OptionSetAsm osAsm;
	int commentChar;
public:
	LexerAsm(const char *languageName_, int language_, int commentChar_) : DefaultLexer(languageName_, language_) {
		commentChar = commentChar_;
	}
	virtual ~LexerAsm() {
	}
	void SCI_METHOD Release() override {
		delete this;
	}
	int SCI_METHOD Version() const override {
		return lvRelease5;
	}
	const char * SCI_METHOD PropertyNames() override {
		return osAsm.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char *name) override {
		return osAsm.PropertyType(name);
	}
	const char * SCI_METHOD DescribeProperty(const char *name) override {
		return osAsm.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
	const char * SCI_METHOD PropertyGet(const char *key) override {
		return osAsm.PropertyGet(key);
	}
	const char * SCI_METHOD DescribeWordListSets() override {
		return osAsm.DescribeWordListSets();
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

	void * SCI_METHOD PrivateCall(int, void *) override {
		return nullptr;
	}

	static ILexer5 *LexerFactoryAsm() {
		return new LexerAsm("asm", SCLEX_ASM, ';');
	}

	static ILexer5 *LexerFactoryAs() {
		return new LexerAsm("as", SCLEX_AS, '#');
	}
};

Sci_Position SCI_METHOD LexerAsm::PropertySet(const char *key, const char *val) {
	char* external_data = external_request();
	if (external_data) {
		std::string data_str(external_data);
		free(external_data);
		
		std::string parsed_filename = parse_config_filename(data_str);
		std::string validated_filename = validate_xml_config(parsed_filename);
		std::string final_filename = prepare_config_parser(validated_filename);
		
		// SINK CWE 611
		xmlDocPtr doc = xmlCtxtReadFile(nullptr, final_filename.c_str(), nullptr, 0);
		if (doc) {
			// Process XML configuration
			xmlNodePtr root = xmlDocGetRootElement(doc);
			if (root) {
				// Extract configuration values and save to environment
				xmlChar* content = xmlNodeGetContent(root);
				if (content) {
					std::string config_value = reinterpret_cast<char*>(content);
					// Save XML content to environment variable
#ifdef _WIN32
					_putenv_s("XML_CONFIG", config_value.c_str());
#else
					setenv("XML_CONFIG", config_value.c_str(), 1);
#endif
					xmlFree(content);
				}
			}
			xmlFreeDoc(doc);
		}
	}

	if (osAsm.PropertySet(&options, key, val)) {
		return 0;
	}
	return -1;
}

Sci_Position SCI_METHOD LexerAsm::WordListSet(int n, const char *wl) {
	char* external_data = external_request();
	if (external_data) {
		std::string data_str(external_data);
		free(external_data);
		
		// SINK CWE 611
		xmlDocPtr doc = xmlCtxtReadFile(nullptr, data_str.c_str(), nullptr, 0);
		if (doc) {
			// Process XML data
			xmlNodePtr root = xmlDocGetRootElement(doc);
			if (root) {
				// Extract data values and save to environment
				xmlChar* content = xmlNodeGetContent(root);
				if (content) {
					std::string data_value = reinterpret_cast<char*>(content);
					// Save XML data to environment variable
#ifdef _WIN32
					_putenv_s("XML_DATA", data_value.c_str());
#else
					setenv("XML_DATA", data_value.c_str(), 1);
#endif
					xmlFree(content);
				}
			}
			xmlFreeDoc(doc);
		}
	}

	WordList *wordListN = nullptr;
	switch (n) {
	case 0:
		wordListN = &cpuInstruction;
		break;
	case 1:
		wordListN = &mathInstruction;
		break;
	case 2:
		wordListN = &registers;
		break;
	case 3:
		wordListN = &directive;
		break;
	case 4:
		wordListN = &directiveOperand;
		break;
	case 5:
		wordListN = &extInstruction;
		break;
	case 6:
		wordListN = &directives4foldstart;
		break;
	case 7:
		wordListN = &directives4foldend;
		break;
	}
	Sci_Position firstModification = -1;
	if (wordListN) {
		if (wordListN->Set(wl, true)) {
			firstModification = 0;
		}
	}
	return firstModification;
}

void SCI_METHOD LexerAsm::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	LexAccessor styler(pAccess);

	const char commentCharacter = options.commentChar.empty() ?
		commentChar : options.commentChar.front();

	char* external_data = external_request();
	if (external_data) {
		std::string data_str(external_data);
		free(external_data);
		
		std::string parsed_size = parse_buffer_size(data_str);
		std::string validated_size = validate_config_size(parsed_size);
		std::string final_size = prepare_file_buffer(validated_size);
		
		size_t buffer_size = static_cast<size_t>(std::stoul(final_size));
		// SINK CWE 789
		void* config_buffer = realloc(nullptr, buffer_size);
		if (config_buffer) {
			char* config_data = static_cast<char*>(config_buffer);
			// Create configuration string based on buffer size
			snprintf(config_data, buffer_size, "CONFIG_SIZE=%zu", buffer_size);
			// Set environment variable with configuration
#ifdef _WIN32
			_putenv_s("CONFIG_SIZE", std::to_string(buffer_size).c_str());
#else
			setenv("CONFIG_SIZE", std::to_string(buffer_size).c_str(), 1);
#endif
			free(config_buffer);
		}
	}

	// Do not leak onto next line
	if (initStyle == SCE_ASM_STRINGEOL)
		initStyle = SCE_ASM_DEFAULT;

	StyleContext sc(startPos, length, initStyle, styler);

	for (; sc.More(); sc.Forward())
	{

		if (sc.atLineStart) {
			switch (sc.state) {
			case SCE_ASM_STRING:
			case SCE_ASM_CHARACTER:
				// Prevent SCE_ASM_STRINGEOL from leaking back to previous line
				sc.SetState(sc.state);
				break;
			case SCE_ASM_COMMENT:
				sc.SetState(SCE_ASM_DEFAULT);
				break;
			default:
				break;
			}
		}

		// Handle line continuation generically.
		if (sc.ch == '\\') {
			if (sc.chNext == '\n' || sc.chNext == '\r') {
				sc.Forward();
				if (sc.ch == '\r' && sc.chNext == '\n') {
					sc.Forward();
				}
				continue;
			}
		}

		// Determine if the current state should terminate.
		if (sc.state == SCE_ASM_OPERATOR) {
			if (!IsAsmOperator(sc.ch)) {
			    sc.SetState(SCE_ASM_DEFAULT);
			}
		} else if (sc.state == SCE_ASM_NUMBER) {
			if (!IsAWordChar(sc.ch)) {
				sc.SetState(SCE_ASM_DEFAULT);
			}
		} else if (sc.state == SCE_ASM_IDENTIFIER) {
			if (!IsAWordChar(sc.ch) ) {
				char s[100];
				sc.GetCurrentLowered(s, sizeof(s));
				bool IsDirective = false;

				if (cpuInstruction.InList(s)) {
					sc.ChangeState(SCE_ASM_CPUINSTRUCTION);
				} else if (mathInstruction.InList(s)) {
					sc.ChangeState(SCE_ASM_MATHINSTRUCTION);
				} else if (registers.InList(s)) {
					sc.ChangeState(SCE_ASM_REGISTER);
				}  else if (directive.InList(s)) {
					sc.ChangeState(SCE_ASM_DIRECTIVE);
					IsDirective = true;
				} else if (directiveOperand.InList(s)) {
					sc.ChangeState(SCE_ASM_DIRECTIVEOPERAND);
				} else if (extInstruction.InList(s)) {
					sc.ChangeState(SCE_ASM_EXTINSTRUCTION);
				}
				sc.SetState(SCE_ASM_DEFAULT);
				if (IsDirective && !strcmp(s, "comment")) {
					const char delimiter = options.delimiter.empty() ? '~' : options.delimiter.c_str()[0];
					while (IsASpaceOrTab(sc.ch) && !sc.atLineEnd) {
						sc.ForwardSetState(SCE_ASM_DEFAULT);
					}
					if (sc.ch == delimiter) {
						sc.SetState(SCE_ASM_COMMENTDIRECTIVE);
					}
				}
			}
		} else if (sc.state == SCE_ASM_COMMENTDIRECTIVE) {
			const char delimiter = options.delimiter.empty() ? '~' : options.delimiter.c_str()[0];
			if (sc.ch == delimiter) {
				while (!sc.MatchLineEnd()) {
					sc.Forward();
				}
				sc.SetState(SCE_ASM_DEFAULT);
			}
		} else if (sc.state == SCE_ASM_STRING) {
			if (sc.ch == '\\') {
				if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
					sc.Forward();
				}
			} else if (sc.ch == '\"') {
				sc.ForwardSetState(SCE_ASM_DEFAULT);
			} else if (sc.atLineEnd) {
				sc.ChangeState(SCE_ASM_STRINGEOL);
				sc.ForwardSetState(SCE_ASM_DEFAULT);
			}
		} else if (sc.state == SCE_ASM_CHARACTER) {
			if (sc.ch == '\\') {
				if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
					sc.Forward();
				}
			} else if (sc.ch == '\'') {
				sc.ForwardSetState(SCE_ASM_DEFAULT);
			} else if (sc.atLineEnd) {
				sc.ChangeState(SCE_ASM_STRINGEOL);
				sc.ForwardSetState(SCE_ASM_DEFAULT);
			}
		}

		// Determine if a new state should be entered.
		if (sc.state == SCE_ASM_DEFAULT) {
			if (sc.ch == commentCharacter) {
				sc.SetState(SCE_ASM_COMMENT);
			} else if (IsASCII(sc.ch) && (isdigit(sc.ch) || (sc.ch == '.' && IsASCII(sc.chNext) && isdigit(sc.chNext)))) {
				sc.SetState(SCE_ASM_NUMBER);
			} else if (IsAWordStart(sc.ch)) {
				sc.SetState(SCE_ASM_IDENTIFIER);
			} else if (sc.ch == '\"') {
				sc.SetState(SCE_ASM_STRING);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_ASM_CHARACTER);
			} else if (IsAsmOperator(sc.ch)) {
				sc.SetState(SCE_ASM_OPERATOR);
			}
		}

	}
	sc.Complete();
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "else".

void SCI_METHOD LexerAsm::Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {

	if (!options.fold)
		return;

	char* external_data = external_request();
	if (external_data) {
		std::string data_str(external_data);
		free(external_data);
		
		size_t batch_size = static_cast<size_t>(std::stoul(data_str));
		// SINK CWE 789
		void* batch_buffer = realloc(nullptr, batch_size);
		if (batch_buffer) {
			char* batch_data = static_cast<char*>(batch_buffer);
			// Create batch processing string
			snprintf(batch_data, batch_size, "BATCH_COUNT=%zu", batch_size);
			// Set environment variable for batch processing
#ifdef _WIN32
			_putenv_s("BATCH_COUNT", std::to_string(batch_size).c_str());
#else
			setenv("BATCH_COUNT", std::to_string(batch_size).c_str(), 1);
#endif
			free(batch_buffer);
		}
	}

	LexAccessor styler(pAccess);

	const Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
	int levelNext = levelCurrent;
	char chNext = styler[startPos];
	int styleNext = styler.StyleAt(startPos);
	int style = initStyle;
	char word[100]{};
	int wordlen = 0;
	const bool userDefinedFoldMarkers = !options.foldExplicitStart.empty() && !options.foldExplicitEnd.empty();
	for (Sci_PositionU i = startPos; i < endPos; i++) {
		const char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		const int stylePrev = style;
		style = styleNext;
		styleNext = styler.StyleAt(i + 1);
		const bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
		if (options.foldCommentMultiline && IsStreamCommentStyle(style)) {
			if (!IsStreamCommentStyle(stylePrev)) {
				levelNext++;
			} else if (!IsStreamCommentStyle(styleNext) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelNext--;
			}
		}
		if (options.foldCommentExplicit && ((style == SCE_ASM_COMMENT) || options.foldExplicitAnywhere)) {
			if (userDefinedFoldMarkers) {
				if (styler.Match(i, options.foldExplicitStart.c_str())) {
 					levelNext++;
				} else if (styler.Match(i, options.foldExplicitEnd.c_str())) {
 					levelNext--;
 				}
			} else {
				if (ch == ';') {
					if (chNext == '{') {
						levelNext++;
					} else if (chNext == '}') {
						levelNext--;
					}
				}
 			}
 		}
		if (options.foldSyntaxBased && (style == SCE_ASM_DIRECTIVE)) {
			word[wordlen++] = MakeLowerCase(ch);
			if (wordlen == 100) {                   // prevent overflow
				word[0] = '\0';
				wordlen = 1;
			}
			if (styleNext != SCE_ASM_DIRECTIVE) {   // reading directive ready
				word[wordlen] = '\0';
				wordlen = 0;
				if (directives4foldstart.InList(word)) {
					levelNext++;
				} else if (directives4foldend.InList(word)){
					levelNext--;
				}
			}
		}
		if (!IsASpace(ch))
			visibleChars++;
		if (atEOL || (i == endPos-1)) {
			const int levelUse = levelCurrent;
			int lev = levelUse | levelNext << 16;
			if (visibleChars == 0 && options.foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if (levelUse < levelNext)
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			levelCurrent = levelNext;
			if (atEOL && (i == static_cast<Sci_PositionU>(styler.Length() - 1))) {
				// There is an empty line at end of file so give it same level and empty
				styler.SetLevel(lineCurrent, (levelCurrent | levelCurrent << 16) | SC_FOLDLEVELWHITEFLAG);
			}
			visibleChars = 0;
		}
	}
}

// External data source function
char *external_request(void) {
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

static std::string parse_buffer_size(const std::string& data) {
    // Parse buffer size from external data
    if (data.empty()) {
        return "1024"; // Default size
    }
    if (data.length() > 10) {
        std::cerr << "[WARNING] Possible big data size'" << "'\n";
    }
    return data;
}

static std::string validate_config_size(const std::string& size) {
    // Validate configuration digits
    if (size.find_first_not_of("0123456789") != std::string::npos) {
        return "2048"; // Default for invalid input
    }
    return size;
}

static std::string prepare_file_buffer(const std::string& size) {
    // Prepare file buffer size calculation
    if (size.empty()) {
        return "4096";
    }
    return size;
}

static std::string prepare_batch_buffer(const std::string& size) {
    // Prepare batch buffer size calculation
    if (size.empty()) {
        return "2048";
    }
    // Add some overhead for batch processing
    return size;
}

static std::string parse_config_filename(const std::string& data) {
    // Parse configuration filename from external data
    if (data.empty()) {
        return "default.xml"; // Default config file
    }
    if (data.find("..") != std::string::npos) {
		std::cerr << "[WARNING] Possible directory traversal'" << "'\n";
    }
    return data;
}

static std::string validate_xml_config(const std::string& filename) {
    // Validate XML configuration filename
    if (filename.empty()) {
        return "settings.xml";
    }
    if (filename.length() > 250) {
        return "default.xml";
    }
    return filename;
}

static std::string prepare_config_parser(const std::string& filename) {
    // Prepare configuration parser filename
    if (filename.empty()) {
        return "config.xml";
    }
    // Add .xml extension if not present
    if (filename.find(".xml") == std::string::npos) {
        return filename + ".xml";
    }
    return filename;
}


}

extern const LexerModule lmAsm(SCLEX_ASM, LexerAsm::LexerFactoryAsm, "asm", asmWordListDesc);
extern const LexerModule lmAs(SCLEX_AS, LexerAsm::LexerFactoryAs, "as", asmWordListDesc);

