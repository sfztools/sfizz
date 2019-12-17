#include <regex>

namespace sfz
{
namespace Regexes 
{
	static std::regex includes { R"V(#include\s*"(.*?)".*$)V", std::regex::optimize };
	static std::regex defines { R"(#define\s*(\$[a-zA-Z0-9_]+)\s+([a-zA-Z0-9-]+)(?=\s|$))", std::regex::optimize };
	static std::regex headers { R"(<(.*?)>(.*?)(?=<|$))", std::regex::optimize };
	static std::regex members { R"(([a-zA-Z0-9_]+)=([a-zA-Z0-9-_#.&\/\s\\\(\),\*]+)(?![a-zA-Z0-9_]*=))", std::regex::optimize };
	static std::regex opcodeParameters { R"(([a-zA-Z0-9_]+?)([0-9]+)$)", std::regex::optimize };
}
}