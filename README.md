# Интерпретатор
## Краткое описание
Учебный проект интерпретатора собственного DSL-языка программирования Mython. Язык поддерживает динамические переменные, классы, наследование, методы, функции, унарные и бинарные операции над переменными, функцию вывода в поток.
## Компиляция
Сборка проекта осуществляется при помощи CMake из корневой папки проекта. Команды для сборки и компиляции:
```
cmake -B./build -G "Unix Makefiles"
cmake --build build
```
## Использование
Пример использования программы:
```
istringstream input(R"(
class Counter:
  def __init__():
    self.value = 0

  def add():
    self.value = self.value + 1

class Dummy:
  def do_add(counter):
    counter.add()

x = Counter()
y = x

x.add()
y.add()

print x.value

d = Dummy()
d.do_add(x)

print y.value
)");

parse::Lexer lexer(input); // Инициилизируем лексер и поток ввода
auto program = ParseProgram(lexer); // Парсим результат работы лексического анализатора

runtime::SimpleContext context{std::cout}; // Задаем контекст (в рамках этой программы задается только поток вывода)
runtime::Closure closure;

program->Execute(closure, context); // Выполняем итоговую программу
```
## Системные требования
* C++17 (STL)
* g++ с поддержкой 17-го стандарта (также, возможно применения иных компиляторов C++ с поддержкой необходимого стандарта)
## Стек
* C++17
* CMake 3.8
