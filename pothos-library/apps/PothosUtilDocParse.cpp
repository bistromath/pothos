// Copyright (c) 2014-2014 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "PothosUtil.hpp"
#include <Pothos/Exception.hpp>
#include <Pothos/Plugin.hpp>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Array.h>
#include <Poco/String.h>
#include <Poco/Format.h>
#include <Poco/Path.h>
#include <Poco/RegularExpression.h>
#include <Poco/StringTokenizer.h>
#include <Poco/NumberFormatter.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cassert>

struct CodeLine
{
    CodeLine(const std::string &text, const size_t lineNo):
        text(text), lineNo(lineNo){}
    std::string text;
    size_t lineNo;
    std::string toString(void) const
    {
        return Poco::format("%d:%s", int(lineNo), text);
    }
};

typedef std::vector<CodeLine> CodeBlock;

/***********************************************************************
 * misc utils
 **********************************************************************/
static std::string titleCase(const std::string &input)
{
    return Poco::toUpper(input.substr(0, 1)) + input.substr(1);
}

static std::vector<std::string> splitCommaArgs(const std::string &argsStr)
{
    std::vector<std::string> args(1);
    int bracketDepth = 0;
    for (char ch : argsStr)
    {
        if (ch == '{' or ch == '[' or ch == '(') bracketDepth++;
        else if (ch == '}' or ch == ']' or ch == ')') bracketDepth--;
        if (ch == ',' and bracketDepth == 0) args.push_back("");
        else args.back().push_back(ch);
        if (bracketDepth < 0) throw Pothos::SyntaxException(
            "bracket mismatch for comma-separated args", argsStr);
    }
    if (args.back().empty()) args.resize(args.size()-1);
    return args;
}

/***********************************************************************
 * Extract contiguous lines of comments
 **********************************************************************/
static std::vector<CodeBlock> extractContiguousBlocks(std::istream &is)
{
    std::vector<CodeBlock> contiguousCommentBlocks;
    CodeBlock currentCodeBlock;
    size_t lineNo = 0;

    bool inMultiLineComment = false;
    while (is.good() and not is.eof())
    {
        lineNo++; //starts at 1
        char buff[4096]; is.getline(buff, 4096);
        const std::string line(buff/*, size_t(is.gcount())*/);
        if (line.empty()) continue;

        const std::string lineTrim = Poco::trimLeft(line);
        const auto openMulti = line.find("/*");
        const auto closeMulti = line.find("*/");
        if (not inMultiLineComment and lineTrim.size() >= 2 and lineTrim.substr(0, 2) == "//")
        {
            currentCodeBlock.push_back(CodeLine(line, lineNo));
        }
        else if (not inMultiLineComment and openMulti != std::string::npos)
        {
            inMultiLineComment = true;
            currentCodeBlock.push_back(CodeLine(line.substr(openMulti, std::string::npos), lineNo));
        }
        else if (inMultiLineComment and closeMulti != std::string::npos)
        {
            inMultiLineComment = false;
            currentCodeBlock.push_back(CodeLine(line.substr(0, closeMulti+2), lineNo));
            is.seekg(is.tellg() + std::streamoff(closeMulti+2-int(line.size())));
        }
        else if (inMultiLineComment)
        {
            currentCodeBlock.push_back(CodeLine(line, lineNo));
        }
        else if (not currentCodeBlock.empty())
        {
            contiguousCommentBlocks.push_back(currentCodeBlock);
            currentCodeBlock.clear();
        }
    }

    if (not currentCodeBlock.empty())
    {
        contiguousCommentBlocks.push_back(currentCodeBlock);
    }

    return contiguousCommentBlocks;
}

/***********************************************************************
 * Strip top and bottom whitespace from a document array
 **********************************************************************/
static Poco::JSON::Array::Ptr stripDocArray(const Poco::JSON::Array &in)
{
    Poco::JSON::Array::Ptr out(new Poco::JSON::Array());

    for (size_t i = 0; i < in.size(); i++)
    {
        //dont add empty lines if the last line is empty
        const auto line = in.get(i).convert<std::string>();
        std::string lastLine;
        if (out->size() != 0) lastLine = out->get(out->size()-1).convert<std::string>();
        if (not lastLine.empty() or not line.empty()) out->add(line);
    }

    //remove trailing empty line from docs
    if (out->size() != 0 and out->get(out->size()-1).convert<std::string>().empty())
    {
        out->remove(out->size()-1);
    }

    return out;
}

/***********************************************************************
 * Parse a single documentation block for markup
 **********************************************************************/
static Poco::JSON::Object parseCommentBlockForMarkup(const CodeBlock &commentBlock)
{
    Poco::JSON::Object topObj;
    Poco::JSON::Array calls;
    Poco::JSON::Array keywords;
    Poco::JSON::Array categories;
    Poco::JSON::Array params;
    Poco::JSON::Array topDocs;
    Poco::JSON::Object::Ptr currentParam;

    std::string state;
    std::string indent;

    std::string instruction;
    std::string payload;

    //search for the markup begin tag and record the indent
    for (const auto &codeLine : commentBlock)
    {
        std::string line = codeLine.text;
        Poco::RegularExpression::MatchVec matches;

        if (not state.empty())
        {
            if (line.size() >= indent.size() and line.substr(0, indent.size()) != indent)
            {
                if (codeLine.lineNo == commentBlock.back().lineNo) line = "";
                else throw Pothos::SyntaxException("Inconsistent indentation", codeLine.toString());
            }

            if (line.size() >= indent.size()) line = line.substr(indent.size());
            else line = "";

            Poco::RegularExpression("^\\|(\\w+)\\s+(.*)$").match(line, 0, matches);
            if (not matches.empty())
            {
                assert(matches.size() == 3);
                instruction = line.substr(matches[1].offset, matches[1].length);
                payload = line.substr(matches[2].offset, matches[2].length);
            }
        }

        if (state.empty())
        {
            Poco::RegularExpression("^(.*)\\|PothosDoc\\s+(.*)$").match(line, 0, matches);
            if (matches.empty()) continue;
            assert(matches.size() == 3);
            indent = line.substr(matches[1].offset, matches[1].length);
            topObj.set("name", Poco::trim(line.substr(matches[2].offset, matches[2].length)));
            state = "DOC";
        }
        else if (matches.empty() and state == "DOC")
        {
            topDocs.add(line);
        }
        else if (matches.empty() and state == "PARAM")
        {
            auto array = currentParam->getArray("desc");
            array->add(line);
            currentParam->set("desc", stripDocArray(*array));
        }
        else if (instruction == "category" and state == "DOC")
        {
            categories.add(Poco::trim(payload));
        }
        else if (instruction == "keywords" and state == "DOC")
        {
            for (const auto &keyword : Poco::StringTokenizer(
                payload, " \t", Poco::StringTokenizer::TOK_TRIM | Poco::StringTokenizer::TOK_IGNORE_EMPTY))
            {
                keywords.add(Poco::trim(keyword));
            }
        }
        else if (instruction == "param" and (state == "DOC" or state == "PARAM"))
        {
            Poco::RegularExpression::MatchVec fields;
            Poco::RegularExpression("^\\s*(\\w+)(\\s*\\[(.*)\\]\\s*)?(.*)$").match(payload, 0, fields);
            if (fields.empty()) throw Pothos::SyntaxException(
                "Expected |param key[name] description",
                codeLine.toString());

            assert(fields.size() == 5);
            const std::string key = Poco::trim(payload.substr(fields[1].offset, fields[1].length));
            std::string name = titleCase(key);
            if (fields[3].length != 0) name = Poco::trim(payload.substr(fields[3].offset, fields[3].length));
            const std::string desc = Poco::trim(payload.substr(fields[4].offset, fields[4].length));

            currentParam = Poco::JSON::Object::Ptr(new Poco::JSON::Object());
            params.add(currentParam);
            currentParam->set("key", key);
            currentParam->set("name", name);
            Poco::JSON::Array::Ptr descArr(new Poco::JSON::Array());
            descArr->add(desc);
            currentParam->set("desc", descArr);
            state = "PARAM";
        }
        else if (instruction == "default" and state == "PARAM")
        {
            if (currentParam->has("default")) throw Pothos::SyntaxException(
                "Multiple occurrence of default for param",
                codeLine.toString());
            currentParam->set("default", payload);
        }
        else if (instruction == "units" and state == "PARAM")
        {
            if (currentParam->has("units")) throw Pothos::SyntaxException(
                "Multiple occurrence of units for param",
                codeLine.toString());
            currentParam->set("units", payload);
        }
        else if (instruction == "preview" and state == "PARAM")
        {
            if (currentParam->has("preview")) throw Pothos::SyntaxException(
                "Multiple occurrence of preview for param",
                codeLine.toString());
            if (payload != "disable" and payload != "enable") throw Pothos::SyntaxException(
                "Only supports enable/disable as value for preview option of param",
                codeLine.toString());
            currentParam->set("preview", payload);
        }
        else if (instruction == "option" and state == "PARAM")
        {
            Poco::RegularExpression::MatchVec fields;
            Poco::RegularExpression("^(\\s*\\[(.*)\\]\\s*)?(.*)$").match(payload, 0, fields);
            if (fields.empty()) throw Pothos::SyntaxException(
                "Expected |option [name] value",
                codeLine.toString());

            assert(fields.size() == 4);
            const std::string value = Poco::trim(payload.substr(fields[3].offset, fields[3].length));
            std::string name = titleCase(value);
            if (fields[2].length != 0) name = Poco::trim(payload.substr(fields[2].offset, fields[2].length));

            Poco::JSON::Object option;
            option.set("value", value);
            option.set("name", name);
            if (not currentParam->has("options")) currentParam->set(
                "options", Poco::JSON::Array::Ptr(new Poco::JSON::Array()));
            currentParam->getArray("options")->add(option);
        }
        else if (instruction == "factory" and (state == "DOC" or state == "PARAM"))
        {
            Poco::RegularExpression::MatchVec fields;
            Poco::RegularExpression("^\\s*(/.*)\\s*\\((.*)\\)$").match(payload, 0, fields);
            if (fields.empty()) throw Pothos::SyntaxException(
                "Expected |factory /registry/path(args, )",
                codeLine.toString());

            assert(fields.size() == 3);
            const std::string path = Poco::trim(payload.substr(fields[1].offset, fields[1].length));
            const std::string argsStr = Poco::trim(payload.substr(fields[2].offset, fields[2].length));

            //add the path
            try {Pothos::PluginPath(path);}
            catch (const Pothos::PluginPathError &)
            {
                throw Pothos::SyntaxException("Invalid factory path", codeLine.toString());
            }
            if (topObj.has("path")) throw Pothos::SyntaxException(
                "Multiple occurrence of factory", codeLine.toString());
            topObj.set("path", path);

            //split and extract args
            Poco::JSON::Array args;
            try
            {
                for (const auto &arg : splitCommaArgs(argsStr)) args.add(Poco::trim(arg));
            }
            catch (const Pothos::Exception &ex)
            {
                throw Pothos::SyntaxException(codeLine.toString(), ex);
            }
            topObj.set("args", args);

            state = "DOC";
        }
        else if (instruction == "setter" and (state == "DOC" or state == "PARAM"))
        {
            Poco::RegularExpression::MatchVec fields;
            Poco::RegularExpression("^\\s*(\\w+)\\s*\\((.*)\\)$").match(payload, 0, fields);
            if (fields.empty()) throw Pothos::SyntaxException(
                "Expected |setter setFooBar(args, )",
                codeLine.toString());

            assert(fields.size() == 3);
            const std::string callName = Poco::trim(payload.substr(fields[1].offset, fields[1].length));
            const std::string argsStr = Poco::trim(payload.substr(fields[2].offset, fields[2].length));

            //split and extract args
            Poco::JSON::Array args;
            try
            {
                for (const auto &arg : splitCommaArgs(argsStr)) args.add(Poco::trim(arg));
            }
            catch (const Pothos::Exception &ex)
            {
                throw Pothos::SyntaxException(codeLine.toString(), ex);
            }

            //add to calls
            Poco::JSON::Object call;
            call.set("name", callName);
            call.set("args", args);
            calls.add(call);

            state = "DOC";
        }
    }

    topObj.set("docs", stripDocArray(topDocs));
    topObj.set("categories", categories);
    topObj.set("keywords", keywords);
    topObj.set("params", params);
    topObj.set("calls", calls);

    //sanity check for required stuff
    if (not state.empty() and not topObj.has("path"))
    {
        throw Pothos::SyntaxException("missing |factory declaration");
    }

    return topObj;
}

/***********************************************************************
 * generate a cpp source that adds the json string to the registry
 **********************************************************************/
static void jsonArrayToCppStaticBlock(const Poco::JSON::Array &arrayOut, std::ostream &os, const std::string &target)
{
    os << "#include <Pothos/Framework.hpp>\n";
    os << "#include <Pothos/Plugin.hpp>\n";
    os << Poco::format("pothos_static_block(registerPothosBlockDocs%s)\n", target);
    os << "{\n";
    for (size_t i = 0; i < arrayOut.size(); i++)
    {
        auto obj = arrayOut.getObject(i);
        assert(obj);

        //create escaped string of json
        std::stringstream ossJsonObj;
        obj->stringify(ossJsonObj);
        std::string escaped;
        for (const auto &ch : ossJsonObj.str())
        {
            escaped += "\\x" + Poco::NumberFormatter::formatHex(int(ch), 2/*width*/, false/*no 0x*/);
        }

        auto path = Pothos::PluginPath("/blocks/docs").join(obj->getValue<std::string>("path").substr(1));
        os << Poco::format("    Pothos::PluginRegistry::add(\"%s\", std::string(\"%s\"));\n", path.toString(), escaped);
    }
    os << "}\n";
}

void PothosUtilBase::docParse(const std::vector<std::string> &inputFilePaths)
{
    Poco::JSON::Array arrayOut;
    for (const auto &inputFilePath : inputFilePaths)
    {

        std::ifstream inputFile(inputFilePath.c_str());
        const auto contiguousBlocks = extractContiguousBlocks(inputFile);
        inputFile.close();

        for (const auto &contiguousBlock : contiguousBlocks)
        {
            try
            {
                Poco::SharedPtr<Poco::JSON::Object> jsonOut(new Poco::JSON::Object());
                *jsonOut = parseCommentBlockForMarkup(contiguousBlock);
                if (jsonOut->has("path")) arrayOut.add(jsonOut);
            }
            catch (const Pothos::Exception &ex)
            {
                throw Pothos::SyntaxException(inputFilePath, ex);
            }
        }
    }

    //write to output (file if specified, otherwise stdout)
    const auto outputFilePath = this->config().getString("outputFile", "");
    const size_t indentSpaces = 4;
    if (outputFilePath.empty())
    {
        std::cout << std::endl;
        Poco::JSON::Stringifier::stringify(arrayOut, std::cout, indentSpaces);
        std::cout << std::endl;
    }
    else
    {
        const auto outputFileName = Poco::Path(outputFilePath).getBaseName();
        const auto outputFileExt = Poco::Path(outputFilePath).getExtension();

        std::ofstream outputFile(outputFilePath.c_str());
        if (outputFileExt == "json") arrayOut.stringify(outputFile, indentSpaces);
        else if (outputFileExt == "cpp") jsonArrayToCppStaticBlock(arrayOut, outputFile, outputFileName);
        else throw Pothos::Exception("PothosUtilBase::docParse()", "unsupported file extension: " + outputFilePath);
        outputFile << std::endl;
        outputFile.close();
    }
}
