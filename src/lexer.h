#pragma once

#include <iosfwd>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace parse {

namespace token_type {
struct Number {  // Лексема «число»
    int value;   // число
};

struct Id {             // Лексема «идентификатор»
    std::string value;  // Имя идентификатора
};

struct Char {    // Лексема «символ»
    char value;  // код символа
};

struct String {  // Лексема «строковая константа»
    std::string value;
};

struct Class {};    // Лексема «class»
struct Return {};   // Лексема «return»
struct If {};       // Лексема «if»
struct Else {};     // Лексема «else»
struct Def {};      // Лексема «def»
struct Newline {};  // Лексема «конец строки»
struct Print {};    // Лексема «print»
struct Indent {};  // Лексема «увеличение отступа», соответствует двум пробелам
struct Dedent {};  // Лексема «уменьшение отступа»
struct Eof {};     // Лексема «конец файла»
struct And {};     // Лексема «and»
struct Or {};      // Лексема «or»
struct Not {};     // Лексема «not»
struct Eq {};      // Лексема «==»
struct NotEq {};   // Лексема «!=»
struct LessOrEq {};     // Лексема «<=»
struct GreaterOrEq {};  // Лексема «>=»
struct None {};         // Лексема «None»
struct True {};         // Лексема «True»
struct False {};        // Лексема «False»
}  // namespace token_type

using TokenBase
    = std::variant<token_type::Number, token_type::Id, token_type::Char, token_type::String,
                   token_type::Class, token_type::Return, token_type::If, token_type::Else,
                   token_type::Def, token_type::Newline, token_type::Print, token_type::Indent,
                   token_type::Dedent, token_type::And, token_type::Or, token_type::Not,
                   token_type::Eq, token_type::NotEq, token_type::LessOrEq, token_type::GreaterOrEq,
                   token_type::None, token_type::True, token_type::False, token_type::Eof>;

struct Token : TokenBase {
    using TokenBase::TokenBase;

    template <typename T>
    [[nodiscard]] bool Is() const {
        return std::holds_alternative<T>(*this);
    }

    template <typename T>
    [[nodiscard]] const T& As() const {
        return std::get<T>(*this);
    }

    template <typename T>
    [[nodiscard]] const T* TryAs() const {
        return std::get_if<T>(this);
    }
};

bool operator==(const Token& lhs, const Token& rhs);
bool operator!=(const Token& lhs, const Token& rhs);

std::ostream& operator<<(std::ostream& os, const Token& rhs);

class LexerError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Lexer {
public:
    explicit Lexer(std::istream& input);

    // Возвращает ссылку на текущий токен или token_type::Eof, если поток токенов закончился
    [[nodiscard]] const Token& CurrentToken() const;

    // Возвращает следующий токен, либо token_type::Eof, если поток токенов закончился
    Token NextToken();

    // Если текущий токен имеет тип T, метод возвращает ссылку на него.
    // В противном случае метод выбрасывает исключение LexerError
    template <typename T>
    const T& Expect() const;

    // Метод проверяет, что текущий токен имеет тип T, а сам токен содержит значение value.
    // В противном случае метод выбрасывает исключение LexerError
    template <typename T, typename U>
    void Expect(const U& value) const;

    // Если следующий токен имеет тип T, метод возвращает ссылку на него.
    // В противном случае метод выбрасывает исключение LexerError
    template <typename T>
    const T& ExpectNext();

    // Метод проверяет, что следующий токен имеет тип T, а сам токен содержит значение value.
    // В противном случае метод выбрасывает исключение LexerError
    template <typename T, typename U>
    void ExpectNext(const U& value);

private:
    std::istream& input_;       // Поток ввода
    int current_indent_ = 0;    // Текущий отступ
    int current_pos_ = -1;      // Текущая выводимая позиция в tokens_
    std::vector<Token> tokens_; // Последовательность токенов

    // Класс является представлением считанной из потока input строки в токенах
    class TokenLine final {
    public:
        explicit TokenLine(std::istream& input);

        // Метод считывает очередную строку из input_,
        // возвращает ссылку на текущий объект
        TokenLine& ReadLine();

        // Метод возвращает true, если строка содержит только пробелы и перевод строки
        bool IsEmpty() const;

        // Метод возвращает отступ строки
        int GetIndent() const;
        // Метод возвращает ссылку на вектор токенов строки
        const std::vector<Token>& GetTokens() const;

    private:
        std::istream& input_;       // Поток ввода
        int line_indent_ = 0;       // Отступ в строке
        std::vector<Token> tokens_; // Последовательность токенов в строке

        // Метод считывает количество отступов от начала строки.
        // Если количество отступов нечетное - выбрасывает исключение LexerError
        void CountIndents();
        // Метод считывает комментарий до конца строки/файла
        void ReadComment();
        // Метод считывает строку из потока ввода, заключенные в кавычки символом ch,
        // возвращает токен типа String. В случае неудачи выбрасывает исключение LexerError
        [[nodiscard]] Token ReadString(char quote) const;
        // Метод считывает число из потока ввода, возвращает токен типа Number.
        // В случае неудачи выбрасывает исключение LexerError
        [[nodiscard]] Token ReadNumber() const;
        // Метод считывает идентификатор из потока ввода, возвращает токен типа Id.
        // В случае неудачи выбрасывает исключение LexerError
        [[nodiscard]] Token ReadId() const;
        // Метод считывает лексему эквивалентности из потока ввода,
        // возвращает токен одного из типов: Eq, NotEq, LessOrEq, GreaterOrEq.
        // В случае неудачи выбрасывает исключение LexerError
        [[nodiscard]] Token ReadEq() const;
    };

    // Метод обновляет отступ и добавляет соответствующие токены при необходимости
    void UpdateIndent(int new_indent);

    TokenLine GetTokenLine(std::istream& input) const;
};

template <typename T>
const T& Lexer::Expect() const {
    using namespace std::literals;

    if (CurrentToken().Is<T>()) {
        return CurrentToken().As<T>();
    }

    throw LexerError("Another type expected"s);
}

template <typename T, typename U>
void Lexer::Expect(const U& value) const {
    using namespace std::literals;

    const T& res = Expect<T>();
    if (res.value != value) {
        throw LexerError("Another value expected"s);
    }
}

template <typename T>
const T& Lexer::ExpectNext() {
    NextToken();
    return Expect<T>();
}

template <typename T, typename U>
void Lexer::ExpectNext(const U& value) {
    NextToken();
    Expect<T, U>(value);
}

}  // namespace parse
