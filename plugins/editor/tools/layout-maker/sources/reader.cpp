// -*- C++ -*-
// SPDX-License-Identifier: BSL-1.0
//
//          Copyright Jean Pierre Cimalando 2019-2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
#include "reader.h"
#include <absl/strings/string_view.h>
#include <iostream>
#include <fstream>

///
class tokenizer {
public:
    tokenizer(
        absl::string_view text,
        absl::string_view dropped_delims,
        absl::string_view kept_delims);

    absl::string_view next(bool consume);
    std::string string_until(char closing, bool consume);
    bool at_end() const;

private:
    bool is_dropped(char c) const {
        return dropped_delims_.find(c) != dropped_delims_.npos;
    }
    bool is_kept(char c) const {
        return kept_delims_.find(c) != kept_delims_.npos;
    }
    bool is_delim(char c) const {
        return dropped_delims_.find(c) != dropped_delims_.npos ||
            kept_delims_.find(c) != kept_delims_.npos;
    }

private:
    absl::string_view text_;
    absl::string_view dropped_delims_;
    absl::string_view kept_delims_;
};

tokenizer::tokenizer(
    absl::string_view text,
    absl::string_view dropped_delims,
    absl::string_view kept_delims)
    : text_(text), dropped_delims_(dropped_delims), kept_delims_(kept_delims)
{
}

absl::string_view tokenizer::next(bool consume)
{
    absl::string_view text = text_;

    while (!text.empty() && is_dropped(text[0]))
        text.remove_prefix(1);

    if (text.empty())
        return {};

    size_t pos;
    {
        auto it = std::find_if(text.begin(), text.end(), [this](char c) -> bool { return is_delim(c); });
        if (it == text.end())
            pos = text.size();
        else {
            pos = std::distance(text.begin(), it);
            pos += is_kept(text[0]);
        }
    }

    absl::string_view token = text.substr(0, pos);
    if (consume)
        text_ = text.substr(pos);
    return token;
}

std::string tokenizer::string_until(char closing, bool consume)
{
    absl::string_view text = text_;
    std::string result;

    result.reserve(256);

    for (char c; !text.empty() &&
             ((c = text[0]), text.remove_prefix(1), c != closing); ) {
        if (c != '\\')
            result.push_back(c);
        else {
            if (!text.empty()) {
                c = text[0];
                text.remove_prefix(1);
                result.push_back(c);
            }
        }
    }

    if (consume)
        text_ = text;
    return result;
}

bool tokenizer::at_end() const
{
    absl::string_view text = text_;

    while (!text.empty() && is_dropped(text[0]))
        text.remove_prefix(1);

    return text.empty();
}

///
typedef std::vector<std::string> TokenList;
static bool read_file_lines(const char *filename, std::string &text);
static Layout read_tokens_layout(tokenizer &tkzr);

Layout read_file_layout(const char *filename)
{
    std::string text;
    if (!read_file_lines(filename, text))
        throw std::runtime_error("Cannot read fluid design file.");

    tokenizer tok(text, " \t\r\n", "{}");
    return read_tokens_layout(tok);
}

static std::string consume_next_token(tokenizer &tkzr)
{
    if (tkzr.at_end())
        throw file_format_error("Premature end of tokens");
    return std::string(tkzr.next(true));
}

static bool try_consume_next_token(const char *text, tokenizer &tkzr)
{
    if (tkzr.at_end())
        return false;

    if (tkzr.next(false) != text)
        return false;

    tkzr.next(true);
    return true;
}

static void ensure_next_token(const char *text, tokenizer &tkzr)
{
    std::string tok = consume_next_token(tkzr);
    if (tok != text)
        throw file_format_error("Unexpected token: " + tok);
}

static std::string consume_any_string(tokenizer &tkzr)
{
    if (!tkzr.at_end() && tkzr.next(false) == "{") {
        tkzr.next(true);
        return tkzr.string_until('}', true);
    }
    else
        return consume_next_token(tkzr);
}

static int consume_int_token(tokenizer &tkzr)
{
    std::string text = consume_next_token(tkzr);
    return std::stoi(text);
}

static int consume_real_token(tokenizer &tkzr)
{
    std::string text = consume_next_token(tkzr);
    return std::stod(text);
}

static void consume_layout_item_properties(LayoutItem &item, tokenizer &tkzr)
{
    ensure_next_token("{", tkzr);
    for (bool have = true; have;) {
        if (try_consume_next_token("open", tkzr))
            ; // skip
        else if (try_consume_next_token("selected", tkzr))
            ; // skip
        else if (try_consume_next_token("label", tkzr))
            item.label = consume_any_string(tkzr);
        else if (try_consume_next_token("xywh", tkzr)) {
            ensure_next_token("{", tkzr);
            item.x = consume_int_token(tkzr);
            item.y = consume_int_token(tkzr);
            item.w = consume_int_token(tkzr);
            item.h = consume_int_token(tkzr);
            ensure_next_token("}", tkzr);
        }
        else if (try_consume_next_token("box", tkzr))
            item.box = consume_next_token(tkzr);
        else if (try_consume_next_token("down_box", tkzr))
            item.down_box = consume_next_token(tkzr);
        else if (try_consume_next_token("labelfont", tkzr))
            item.labelfont = consume_int_token(tkzr);
        else if (try_consume_next_token("labelsize", tkzr))
            item.labelsize = consume_int_token(tkzr);
        else if (try_consume_next_token("labeltype", tkzr))
            item.labeltype = consume_any_string(tkzr);
        else if (try_consume_next_token("textsize", tkzr))
            item.textsize = consume_int_token(tkzr);
        else if (try_consume_next_token("align", tkzr))
            item.align = consume_int_token(tkzr);
        else if (try_consume_next_token("type", tkzr))
            item.type = consume_any_string(tkzr);
        else if (try_consume_next_token("callback", tkzr))
            item.callback = consume_any_string(tkzr);
        else if (try_consume_next_token("class", tkzr))
            item.classname = consume_any_string(tkzr);
        else if (try_consume_next_token("value", tkzr))
            item.value = consume_real_token(tkzr);
        else if (try_consume_next_token("minimum", tkzr))
            item.minimum = consume_real_token(tkzr);
        else if (try_consume_next_token("maximum", tkzr))
            item.maximum = consume_real_token(tkzr);
        else if (try_consume_next_token("step", tkzr))
            item.step = consume_real_token(tkzr);
        else if (try_consume_next_token("image", tkzr))
            item.image.filepath = consume_any_string(tkzr);
        else if (try_consume_next_token("hide", tkzr))
            item.hidden = true;
        else if (try_consume_next_token("visible", tkzr))
            /* skip */;
        else if (try_consume_next_token("comment", tkzr))
            item.comment = consume_any_string(tkzr);
        else
            have = false;
    }
    ensure_next_token("}", tkzr);
}

static LayoutItem consume_layout_item(const std::string &classname, tokenizer &tkzr, bool anonymous = false)
{
    LayoutItem item;
    item.classname = classname;
    if (!anonymous)
        item.id = consume_any_string(tkzr);
    consume_layout_item_properties(item, tkzr);
    if (!tkzr.at_end() && tkzr.next(false) == "{") {
        consume_next_token(tkzr);
        for (std::string text; (text = consume_next_token(tkzr)) != "}";) {
            if (text == "decl") {
                consume_any_string(tkzr);
                consume_any_string(tkzr);
            }
            else if (text == "Function") {
                consume_any_string(tkzr);
                consume_any_string(tkzr);
                consume_any_string(tkzr);
            }
            else
                item.items.push_back(consume_layout_item(text, tkzr));
        }
    }
    return item;
}

static Layout read_tokens_layout(tokenizer &tkzr)
{
    Layout layout;

    std::string version_name;
    std::string header_name;
    std::string code_name;

    while (!tkzr.at_end()) {
        std::string key = consume_next_token(tkzr);

        if (key == "version")
            version_name = consume_next_token(tkzr);
        else if (key == "header_name") {
            ensure_next_token("{", tkzr);
            header_name = consume_next_token(tkzr);
            ensure_next_token("}", tkzr);
        }
        else if (key == "code_name") {
            ensure_next_token("{", tkzr);
            code_name = consume_next_token(tkzr);
            ensure_next_token("}", tkzr);
        }
        else if (key == "decl") {
            consume_any_string(tkzr);
            consume_any_string(tkzr);
        }
        else if (key == "widget_class") {
            key = consume_next_token(tkzr);
            layout.items.push_back(consume_layout_item(key, tkzr, true));
            layout.items.back().id = key;
        }
        else
            layout.items.push_back(consume_layout_item(key, tkzr));
    }

    return layout;
}

///
static bool read_file_lines(const char *filename, std::string &text)
{
    std::ifstream stream(filename);
    std::string line;

    while (std::getline(stream, line)) {
        if (!line.empty() && line[0] != '#') {
            text.append(line);
            text.push_back('\n');
        }
    }

    return !stream.bad();
}
