#include "lexer.h"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <unordered_map>

#include "iostream" // DEBUG

using namespace std;

namespace parse {

bool operator==(const Token& lhs, const Token& rhs) {
    using namespace token_type;

    if (lhs.index() != rhs.index()) {
        return false;
    }
    if (lhs.Is<Char>()) {
        return lhs.As<Char>().value == rhs.As<Char>().value;
    }
    if (lhs.Is<Number>()) {
        return lhs.As<Number>().value == rhs.As<Number>().value;
    }
    if (lhs.Is<String>()) {
        return lhs.As<String>().value == rhs.As<String>().value;
    }
    if (lhs.Is<Id>()) {
        return lhs.As<Id>().value == rhs.As<Id>().value;
    }
    return true;
}

bool operator!=(const Token& lhs, const Token& rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const Token& rhs) {
    using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

    VALUED_OUTPUT(Number);
    VALUED_OUTPUT(Id);
    VALUED_OUTPUT(String);
    VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

    UNVALUED_OUTPUT(Class);
    UNVALUED_OUTPUT(Return);
    UNVALUED_OUTPUT(If);
    UNVALUED_OUTPUT(Else);
    UNVALUED_OUTPUT(Def);
    UNVALUED_OUTPUT(Newline);
    UNVALUED_OUTPUT(Print);
    UNVALUED_OUTPUT(Indent);
    UNVALUED_OUTPUT(Dedent);
    UNVALUED_OUTPUT(And);
    UNVALUED_OUTPUT(Or);
    UNVALUED_OUTPUT(Not);
    UNVALUED_OUTPUT(Eq);
    UNVALUED_OUTPUT(NotEq);
    UNVALUED_OUTPUT(LessOrEq);
    UNVALUED_OUTPUT(GreaterOrEq);
    UNVALUED_OUTPUT(None);
    UNVALUED_OUTPUT(True);
    UNVALUED_OUTPUT(False);
    UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

    return os << "Unknown token :("sv;
}

Lexer::Lexer(std::istream& input)
    : input_(input)
{
    NextToken();
}

const Token& Lexer::CurrentToken() const {
    assert(!tokens_.empty());
    return tokens_.at(current_pos_);
}

Token Lexer::NextToken() {
    if ((current_pos_ + 1) < static_cast<int>(tokens_.size())) {
        return tokens_.at(++current_pos_);
    }

    if (!tokens_.empty() && tokens_.back().Is<token_type::Eof>()) {
        return tokens_.back();
    }

    while (true) {
        TokenLine token_line = GetTokenLine(input_);

        if (token_line.IsEmpty()) {
            continue;
        }

        UpdateIndent(token_line.GetIndent());
        tokens_.insert(tokens_.end(), token_line.GetTokens().begin(), token_line.GetTokens().end());
        break;
    }

    return tokens_.at(++current_pos_);
}

Lexer::TokenLine::TokenLine(std::istream& input) : input_(input) {}

Lexer::TokenLine& Lexer::TokenLine::ReadLine() {
    CountIndents();

    char ch;
    while (!input_.eof() && input_.get(ch)) {
        if (ch == '\n') {
            tokens_.emplace_back(token_type::Newline());
            break;
        }
        if (ch == ' ') {
            continue;
        }

        if (ch == '#') {
            ReadComment();
        }
        else if (ch == '"' || ch == '\'') {
            tokens_.push_back(ReadString(ch));
        }
        else if (isdigit(ch)) {
            input_.putback(ch);
            tokens_.push_back(ReadNumber());
        }
        else if (isalpha(ch) || ch == '_') {
            input_.putback(ch);
            tokens_.push_back(ReadId());
        }
        else if ((ch == '=' || ch == '!' || ch == '<' || ch == '>')
            && input_.peek() == '=')
        {
            input_.putback(ch);
            tokens_.push_back(ReadEq());
        }
        else {
            tokens_.push_back(token_type::Char{ ch });
        }
    }

    // Если был достигнут конец потока - добавляем соответствующую лексему в tokens_
    if (input_.eof()) {
        if (!tokens_.empty() && !tokens_.back().Is<token_type::Newline>()) {
            tokens_.emplace_back(token_type::Newline());
        }
        tokens_.emplace_back(token_type::Eof());
    }

    return *this;
}

void Lexer::TokenLine::CountIndents() {
    while (input_.peek() == ' ') {
        ++line_indent_;
        input_.get();
    }

    if (line_indent_ % 2 != 0) {
        throw LexerError("Wrong indents number"s);
    }

    line_indent_ /= 2;
}
void Lexer::TokenLine::ReadComment() {
    while (input_.get() != '\n' && !input_.eof()) {}
    if (!input_.eof()) {
        input_.unget();
    }

}
Token Lexer::TokenLine::ReadString(char quote) const {
    auto it = istreambuf_iterator<char>(input_);
    auto end_it = istreambuf_iterator<char>();

    token_type::String line;

    while (true) {
        // Если строка закончилась без знака '"' или '\''
        // - выбрасываем исключение LexerError
        if (it == end_it) {
            throw LexerError("String parsing error"s);
        }

        const char ch = *it;

        // Если строка закончилась корректным символом - выходим из цикла
        if (ch == quote) {
            ++it;
            break;
        }

        // Если встретился символ '\' - следующим должен идти
        // один из спецсимволов: 'n', 't', '"', '\''
        if (ch == '\\') {
            ++it;

            if (it == end_it) {
                throw LexerError("String parsing error"s);
            }

            const char spec_char = *it;

            switch(spec_char) {
            case 'n':
                line.value.push_back('\n');
                break;
            case 't':
                line.value.push_back('\t');
                break;
            case '"':
                line.value.push_back('"');
                break;
            case '\'':
                line.value.push_back('\'');
                break;
            default:
                throw LexerError("Wrong special symbol appeared"s);
            }
            ++it;
        }
        // Если строка внезапно прерывается - выбрасываем исключение
        else if (ch == '\n' || ch == '\r') {
            throw LexerError("Unexpected end of line"s);
        }
        else {
            ++it;
            line.value.push_back(ch);
        }
    }

    return line;
}
Token Lexer::TokenLine::ReadNumber() const {
    token_type::Number num;
    string num_str;

    // Вносим минус при наличии
    if (input_.peek() == '-') {
        input_.get();
        num_str.push_back('-');
    }
    // Последовательно считываем все цифры
    while (isdigit(input_.peek())) {
        num_str.push_back(input_.get());
    }

    // Преобразовываем получившуюся строку в число.
    // В случае неудачи - выбрасываем исключение LexerError
    try {
        num.value = stoi(num_str);
    }
    catch (...) {
        throw LexerError("Number parsing error"s);
    }

    return num;
}
Token Lexer::TokenLine::ReadId() const {
    string id = "";
    char ch;

    while (input_.get(ch)) {
        if (!isalpha(ch) && !isdigit(ch) && ch != '_') {
            input_.putback(ch);
            break;
        }
        id.push_back(ch);
    }

    if (id.empty()) {
        throw LexerError("Id parsing error occured"s);
    }

    // Проверяем идентификатор на соответствие ключевым словам
    if (id == "class"s) {
        return token_type::Class();
    }
    if (id == "return"s) {
        return token_type::Return();
    }
    if (id == "if"s) {
        return token_type::If();
    }
    if (id == "return"s) {
        return token_type::Return();
    }
    if (id == "else"s) {
        return token_type::Else();
    }
    if (id == "def"s) {
        return token_type::Def();
    }
    if (id == "print"s) {
        return token_type::Print();
    }
    if (id == "and"s) {
        return token_type::And();
    }
    if (id == "or"s) {
        return token_type::Or();
    }
    if (id == "not"s) {
        return token_type::Not();
    }
    if (id == "None"s) {
        return token_type::None();
    }
    if (id == "True"s) {
        return token_type::True();
    }
    if (id == "False"s) {
        return token_type::False();
    }

    // Если идентификатор не соответствует ни одному ключевому слову - создаем новый
    return token_type::Id{ id };
}
Token Lexer::TokenLine::ReadEq() const {
    char ch = input_.get();
    input_.get(); // Считываем из потока и второй символ

    switch (ch) {
    case '=':
        return token_type::Eq();
    case '!':
        return token_type::NotEq();
    case '<':
        return token_type::LessOrEq();
    case '>':
        return token_type::GreaterOrEq();
    }

    throw LexerError("Unknown equivalence parsing error occured"s);
}

bool Lexer::TokenLine::IsEmpty() const {
    return tokens_.empty()
        || all_of(tokens_.begin(), tokens_.end(), [](const Token& token) {
            return token.Is<token_type::Newline>();
        });
}

int Lexer::TokenLine::GetIndent() const {
    return line_indent_;
}
const std::vector<Token>& Lexer::TokenLine::GetTokens() const {
    return tokens_;
}

void Lexer::UpdateIndent(int new_indent) {
    if (new_indent > current_indent_) {
        for (int i = new_indent - current_indent_; i > 0; --i) {
            tokens_.emplace_back(token_type::Indent());
        }
    }
    else if (new_indent < current_indent_) {
        for (int i = current_indent_ - new_indent; i > 0; --i) {
            tokens_.emplace_back(token_type::Dedent());
        }
    }

    current_indent_ = new_indent;
}

Lexer::TokenLine Lexer::GetTokenLine(std::istream& input) const {
    TokenLine token_line(input);
    return token_line.ReadLine();
}

}  // namespace parse
