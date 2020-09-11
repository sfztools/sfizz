#include "reader.h"
#include <absl/strings/string_view.h>
#include <iostream>
#include <fstream>

typedef std::vector<std::string> TokenList;
static bool read_file_tokens(const char *filename, TokenList &tokens);
static Layout read_tokens_layout(TokenList::iterator &tok_it, TokenList::iterator tok_end);

Layout read_file_layout(const char *filename)
{
    std::vector<std::string> tokens;
    if (!read_file_tokens(filename, tokens))
        throw std::runtime_error("Cannot read fluid design file.");

    TokenList::iterator tok_it = tokens.begin();
    TokenList::iterator tok_end = tokens.end();
    return read_tokens_layout(tok_it, tok_end);
}

static std::string consume_next_token(TokenList::iterator &tok_it, TokenList::iterator tok_end)
{
    if (tok_it == tok_end)
        throw file_format_error("Premature end of tokens");
    return *tok_it++;
}

static bool try_consume_next_token(const char *text, TokenList::iterator &tok_it, TokenList::iterator tok_end)
{
    if (tok_it == tok_end)
        return false;

    if (*tok_it != text)
        return false;

    ++tok_it;
    return true;
}

static void ensure_next_token(const char *text, TokenList::iterator &tok_it, TokenList::iterator tok_end)
{
    std::string tok = consume_next_token(tok_it, tok_end);
    if (tok != text)
        throw file_format_error("Unexpected token: " + tok);
}

static std::string consume_enclosed_string(TokenList::iterator &tok_it, TokenList::iterator tok_end)
{
    ensure_next_token("{", tok_it, tok_end);
    unsigned depth = 1;

    std::string text;
    for (;;) {
        std::string part = consume_next_token(tok_it, tok_end);
        if (part == "}") {
            if (--depth == 0)
                return text;
        }
        else if (part == "{")
            ++depth;
        if (!text.empty())
            text.push_back(' ');
        text.append(part);
    }

    return text;
}

static std::string consume_any_string(TokenList::iterator &tok_it, TokenList::iterator tok_end)
{
    if (tok_it != tok_end && *tok_it == "{")
        return consume_enclosed_string(tok_it, tok_end);
    else
        return consume_next_token(tok_it, tok_end);
}

static int consume_int_token(TokenList::iterator &tok_it, TokenList::iterator tok_end)
{
    std::string text = consume_next_token(tok_it, tok_end);
    return std::stoi(text);
}

static int consume_real_token(TokenList::iterator &tok_it, TokenList::iterator tok_end)
{
    std::string text = consume_next_token(tok_it, tok_end);
    return std::stod(text);
}

static void consume_image_properties(LayoutImage &image, TokenList::iterator &tok_it, TokenList::iterator tok_end)
{
    for (bool have = true; have;) {
        if (try_consume_next_token("xywh", tok_it, tok_end)) {
            ensure_next_token("{", tok_it, tok_end);
            image.x = consume_int_token(tok_it, tok_end);
            image.y = consume_int_token(tok_it, tok_end);
            image.w = consume_int_token(tok_it, tok_end);
            image.h = consume_int_token(tok_it, tok_end);
            ensure_next_token("}", tok_it, tok_end);
        }
        else
            have = false;
    }
}

// static void consume_layout_item_properties(LayoutItem &item, TokenList::iterator &tok_it, TokenList::iterator tok_end)
// {
//     ensure_next_token("{", tok_it, tok_end);
//     for (std::string text; (text = consume_next_token(tok_it, tok_end)) != "}";) {
//         if (text == "open" || text == "selected")
//             ; // skip
//         else if (text == "label")
//             item.label = consume_any_string(tok_it, tok_end);
//         else if (text == "xywh") {
//             ensure_next_token("{", tok_it, tok_end);
//             item.x = consume_int_token(tok_it, tok_end);
//             item.y = consume_int_token(tok_it, tok_end);
//             item.w = consume_int_token(tok_it, tok_end);
//             item.h = consume_int_token(tok_it, tok_end);
//             ensure_next_token("}", tok_it, tok_end);
//         }
//         else if (text == "box")
//             item.box = consume_next_token(tok_it, tok_end);
//         else if (text == "labelfont")
//             item.labelfont = consume_int_token(tok_it, tok_end);
//         else if (text == "labelsize")
//             item.labelsize = consume_int_token(tok_it, tok_end);
//         else if (text == "labeltype")
//             item.labeltype = consume_any_string(tok_it, tok_end);
//         else if (text == "align")
//             item.align = consume_int_token(tok_it, tok_end);
//         else if (text == "type")
//             item.type = consume_any_string(tok_it, tok_end);
//         else if (text == "callback")
//             item.callback = consume_any_string(tok_it, tok_end);
//         else if (text == "class")
//             item.classname = consume_any_string(tok_it, tok_end);
//         else if (text == "minimum")
//             item.minimum = consume_real_token(tok_it, tok_end);
//         else if (text == "maximum")
//             item.maximum = consume_real_token(tok_it, tok_end);
//         else if (text == "step")
//             item.step = consume_real_token(tok_it, tok_end);
//         else if (text == "image") {
//             item.image.filepath = consume_any_string(tok_it, tok_end);
//             consume_image_properties(item.image, tok_it, tok_end);
//         }
//     }
// }

static void consume_layout_item_properties(LayoutItem &item, TokenList::iterator &tok_it, TokenList::iterator tok_end)
{
    ensure_next_token("{", tok_it, tok_end);
    for (bool have = true; have;) {
        if (try_consume_next_token("open", tok_it, tok_end))
            ; // skip
        else if (try_consume_next_token("selected", tok_it, tok_end))
            ; // skip
        else if (try_consume_next_token("label", tok_it, tok_end))
            item.label = consume_any_string(tok_it, tok_end);
        else if (try_consume_next_token("xywh", tok_it, tok_end)) {
            ensure_next_token("{", tok_it, tok_end);
            item.x = consume_int_token(tok_it, tok_end);
            item.y = consume_int_token(tok_it, tok_end);
            item.w = consume_int_token(tok_it, tok_end);
            item.h = consume_int_token(tok_it, tok_end);
            ensure_next_token("}", tok_it, tok_end);
        }
        else if (try_consume_next_token("box", tok_it, tok_end))
            item.box = consume_next_token(tok_it, tok_end);
        else if (try_consume_next_token("down_box", tok_it, tok_end))
            item.down_box = consume_next_token(tok_it, tok_end);
        else if (try_consume_next_token("labelfont", tok_it, tok_end))
            item.labelfont = consume_int_token(tok_it, tok_end);
        else if (try_consume_next_token("labelsize", tok_it, tok_end))
            item.labelsize = consume_int_token(tok_it, tok_end);
        else if (try_consume_next_token("labeltype", tok_it, tok_end))
            item.labeltype = consume_any_string(tok_it, tok_end);
        else if (try_consume_next_token("textsize", tok_it, tok_end))
            item.textsize = consume_int_token(tok_it, tok_end);
        else if (try_consume_next_token("align", tok_it, tok_end))
            item.align = consume_int_token(tok_it, tok_end);
        else if (try_consume_next_token("type", tok_it, tok_end))
            item.type = consume_any_string(tok_it, tok_end);
        else if (try_consume_next_token("callback", tok_it, tok_end))
            item.callback = consume_any_string(tok_it, tok_end);
        else if (try_consume_next_token("class", tok_it, tok_end))
            item.classname = consume_any_string(tok_it, tok_end);
        else if (try_consume_next_token("value", tok_it, tok_end))
            item.value = consume_real_token(tok_it, tok_end);
        else if (try_consume_next_token("minimum", tok_it, tok_end))
            item.minimum = consume_real_token(tok_it, tok_end);
        else if (try_consume_next_token("maximum", tok_it, tok_end))
            item.maximum = consume_real_token(tok_it, tok_end);
        else if (try_consume_next_token("step", tok_it, tok_end))
            item.step = consume_real_token(tok_it, tok_end);
        else if (try_consume_next_token("image", tok_it, tok_end))
            item.image.filepath = consume_any_string(tok_it, tok_end);
        else if (try_consume_next_token("hide", tok_it, tok_end))
            item.hidden = true;
        else if (try_consume_next_token("visible", tok_it, tok_end))
            /* skip */;
        else if (try_consume_next_token("comment", tok_it, tok_end))
            item.comment = consume_any_string(tok_it, tok_end);
        else
            have = false;
    }
    ensure_next_token("}", tok_it, tok_end);
}

static LayoutItem consume_layout_item(const std::string &classname, TokenList::iterator &tok_it, TokenList::iterator tok_end, bool anonymous = false)
{
    LayoutItem item;
    item.classname = classname;
    if (!anonymous)
        item.id = consume_any_string(tok_it, tok_end);
    consume_layout_item_properties(item, tok_it, tok_end);
    if (tok_it != tok_end && *tok_it == "{") {
        consume_next_token(tok_it, tok_end);
        for (std::string text; (text = consume_next_token(tok_it, tok_end)) != "}";) {
            if (text == "decl") {
                consume_any_string(tok_it, tok_end);
                consume_any_string(tok_it, tok_end);
            }
            else if (text == "Function") {
                consume_any_string(tok_it, tok_end);
                consume_any_string(tok_it, tok_end);
                consume_any_string(tok_it, tok_end);
            }
            else
                item.items.push_back(consume_layout_item(text, tok_it, tok_end));
        }
    }
    return item;
}

static Layout read_tokens_layout(TokenList::iterator &tok_it, TokenList::iterator tok_end)
{
    Layout layout;

    std::string version_name;
    std::string header_name;
    std::string code_name;

    while (tok_it != tok_end) {
        std::string key = consume_next_token(tok_it, tok_end);

        if (key == "version")
            version_name = consume_next_token(tok_it, tok_end);
        else if (key == "header_name") {
            ensure_next_token("{", tok_it, tok_end);
            header_name = consume_next_token(tok_it, tok_end);
            ensure_next_token("}", tok_it, tok_end);
        }
        else if (key == "code_name") {
            ensure_next_token("{", tok_it, tok_end);
            code_name = consume_next_token(tok_it, tok_end);
            ensure_next_token("}", tok_it, tok_end);
        }
        else if (key == "decl") {
            consume_any_string(tok_it, tok_end);
            consume_any_string(tok_it, tok_end);
        }
        else if (key == "widget_class") {
            key = consume_next_token(tok_it, tok_end);
            layout.items.push_back(consume_layout_item(key, tok_it, tok_end, true));
            layout.items.back().id = key;
        }
        else
            layout.items.push_back(consume_layout_item(key, tok_it, tok_end));
    }

    return layout;
}

///
class tokenizer {
public:
    tokenizer(
        absl::string_view text,
        absl::string_view dropped_delims,
        absl::string_view kept_delims);

    absl::string_view next();

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

absl::string_view tokenizer::next()
{
    auto is_dropped = [this](char c) -> bool {
        return dropped_delims_.find(c) != dropped_delims_.npos;
    };
    auto is_kept = [this](char c) -> bool {
        return kept_delims_.find(c) != kept_delims_.npos;
    };
    auto is_delim = [this](char c) -> bool {
        return dropped_delims_.find(c) != dropped_delims_.npos ||
            kept_delims_.find(c) != kept_delims_.npos;
    };

    absl::string_view text = text_;

    while (!text.empty() && is_dropped(text[0]))
        text.remove_prefix(1);

    if (text.empty())
        return {};

    size_t pos;
    {
        auto it = std::find_if(text.begin(), text.end(), is_delim);
        if (it == text.end())
            pos = text.size();
        else {
            pos = std::distance(text.begin(), it);
            pos += is_kept(text[0]);
        }
    }

    absl::string_view token = text.substr(0, pos);
    text_ = text.substr(pos);
    return token;
}

///
static bool read_file_tokens(const char *filename, TokenList &tokens)
{
    std::ifstream stream(filename);
    std::string line;

    std::string text;
    while (std::getline(stream, line)) {
        if (!line.empty() && line[0] != '#') {
            text.append(line);
            text.push_back('\n');
        }
    }

    if (stream.bad())
        return false;

    tokenizer tok(text, " \t\r\n", "{}");
    absl::string_view token;
    while (!(token = tok.next()).empty())
        tokens.emplace_back(token);

    return !stream.bad();
}
