# Стиль кода conGame

Этот документ описывает стиль, в котором написан исходный код проекта (2023 год).
Новый код и правки должны выглядеть так, будто их писал тот же автор.
Отступать от стиля можно только там, где старый код содержал ошибки
(см. раздел «Что исправляем, а не копируем»).

## Язык и окружение

- C++ (стандарт C++11 и новее: используются инициализаторы членов класса в объявлении).
- Сборка — CMake, IDE — KDevelop / Qt Creator.
- Консольная графика — ncurses, сеть — Boost.Asio.
- Отступ — 4 пробела, без табов. Переводы строк — LF.

## Структура проекта

- Каждый модуль — отдельная папка с именем в PascalCase (`ConRender/`, `Network/`,
  `World/`, `Game/`)
  со своим `CMakeLists.txt` и статической библиотекой `<имя>lib`
  (`conrenderlib`, `networklib`).
- Внутри модуля файлы в нижнем регистре: `conrender.h` / `conrender.cpp`,
  `network.h` / `network.cpp`. Один основной класс на пару файлов.
- Корневой `CMakeLists.txt` подключает модули через `add_subdirectory`
  и линкует их к исполняемому файлу `congame`.
- Подключение чужих модулей — с путём от корня проекта (корень добавлен
  в `include_directories`): `#include "ConRender/conrender.h"`.
- Внутри модуля — просто `#include "conrender.h"`.

### CMake модуля

```cmake
cmake_minimum_required(VERSION 3.0)

project(networklib)

#############################
find_package(Boost COMPONENTS system REQUIRED)
if(NOT Boost_FOUND)
    message(SEND_ERROR "Failed to find Boost")
    return()
else()
    include_directories(${Boost_INCLUDE_DIRS})
endif()
#############################

set(SOURCES_LIB network.cpp)
set(HEADERS_LIB network.h)

add_library(networklib STATIC ${SOURCES_LIB} ${HEADERS_LIB})

target_link_libraries(networklib ${Boost_LIBRARIES})
```

Поиск зависимостей выделяется строками из `#`, исходники и заголовки
собираются в переменные `SOURCE(S)_LIB` / `HEADER(S)_LIB`.

## Именование

| Что                    | Стиль                              | Примеры                                        |
|------------------------|------------------------------------|------------------------------------------------|
| Классы, структуры      | PascalCase                         | `ConRender`, `NetworkServer`, `NetworkClient`  |
| Методы                 | camelCase, глагол + существительное | `clearScr`, `refreshScr`, `connectTo`, `acceptConnection`, `getInput` |
| Сокращения в именах    | допустимы и приветствуются         | `Scr` (screen), `xy` в `printxy`               |
| Параметры, локальные   | camelCase                          | `isServer`, `inp`, `endpoint`                  |
| Объекты в `main`       | camelCase + суффикс роли           | `networkC`, `networkS`, `renderer`             |
| Include guard          | `ИМЯФАЙЛА_H`                       | `CONRENDER_H`, `NETWORK_H`                     |
| Перечисления           | `enum class`, всё в PascalCase     | `Tile::DeepWater`, `Tile::Grass`               |
| Константы              | `const` + camelCase, без `#define` | `chunkSize`, `defaultPort`, `frameTimeMs`      |

Поля класса в старом коде именуются по-разному (`m_PoseX`, `windowWidth`,
`IOservice`, `clientSockets`). Правило: **в существующем классе следовать тому,
как названы соседние поля этого класса; существующие имена не переименовывать.**
В новых классах — camelCase без префикса (`port`, `windowWidth`), как в большинстве полей.

Существующие опечатки в публичных именах (`setViewPosexy`) не трогать без просьбы
автора. В новых именах писать грамотно: `receive`, а не `recive`.

## Форматирование

### Фигурные скобки

Стиль Allman: открывающая скобка на новой строке для классов, функций,
методов и блоков `if` / `for`.

```cpp
bool NetworkServer::acceptConnection()
{
    ...
}
```

Исключение — `main`, у него скобка на той же строке:

```cpp
int main(int argc, char **argv) {
```

`else` пишется на одной строке с закрывающей скобкой:

```cpp
if(inp == 's')
{
    networkS.acceptConnection();
} else if (inp == 'c')
{
    networkC.connectTo("127.0.0.1");
}
```

### Пробелы

- `if(` — без пробела, `else if (` и `for (` — с пробелом.
- В `ConRender` вызовы оформлены с пробелом перед скобкой и пробелами внутри скобок:
  `curs_set ( 0 );`, `getmaxyx ( stdscr, windowHeight, windowWidth );`,
  `initscr ();`. В остальных модулях — обычные вызовы `connect(endpoint)`.
  **Следовать стилю файла, который редактируется.**
- Пробел перед скобкой при конструировании объекта:
  `NetworkServer networkS (9999);`, `new ip::tcp::socket (IOservice)`.
- Пробелы вокруг `->`: `this -> port = port;`, `sock -> connect(endpoint, error);`.
- Пробел перед угловыми скобками шаблона: `std::vector <boost::asio::ip::tcp::socket>`.
- Пустая строка между определениями функций, пустая строка после `#include`.

### switch

`case` на одном уровне со `switch`, тело — с отступом, `break` в каждом `case`:

```cpp
switch(key)
{
case KEY_UP:
case 'w':
    movePlayer(0, 1);
    break;
case 'q':
    running = false;
    break;
}
```

### Многострочные вызовы

Аргументы выравниваются под первым аргументом:

```cpp
mvprintw( -y + m_PoseY + windowHeight/2,
          x + m_PoseX + windowWidth/2,
          ...);
```

## Классы

- Сначала `public`, потом `private`.
- Порядок в `public`: конструктор, методы, деструктор — **последним**.
- Связанные методы группируются, между группами — пустая строка.
- Поля инициализируются прямо в объявлении, если значение по умолчанию известно:
  `int m_PoseX = 0;`.
- Простые наборы данных без поведения — `struct` с публичными полями.
  Конструктор со списком инициализации у `struct` допустим, только если поле
  иначе не создать (сокет Boost.Asio требует `io_context`).
- Методы, которые ещё не реализованы, допустимо оставлять закомментированными
  в заголовке как план (`/* void send (char* str); */`). Реализованные — раскомментировать.
- В конструкторе с одноимёнными параметрами — `this -> port = port;`.

## Прочее

- `using namespace boost::asio;` — допустим в `.cpp`, **никогда** в `.h`.
  В заголовках — полные имена (`boost::asio::ip::tcp::socket`).
- Строки передаются как `std::string`. Для новых функций допустим
  `const std::string&` — это не ломает стиль.
- Комментариев мало: код должен читаться сам. Комментарий только там,
  где без него непонятно *зачем*. В `.h` после `#endif` — `// ИМЯ_H`.
- Сообщения коммитов — на английском, коротко, с заглавной буквы, без точки:
  `Added conRender class`, `Extended Network library`, `Some Fixes`.

## Что исправляем, а не копируем

Старый код содержал ошибки. Их не нужно воспроизводить в новом коде,
а при правке — нужно исправлять:

1. **Каждому `new` — свой `delete`.** Указатели-члены (`sock`,
   `acceptor`) инициализируются `nullptr` в объявлении, освобождаются
   в деструкторе, перед разыменованием проверяются на `nullptr`.
   Сырые указатели как таковые — часть стиля и разрешены; `std::unique_ptr`
   тоже допустим, если так проще.
2. **Классы, владеющие ресурсами** (указатели, ncurses, сокеты), запрещают
   копирование: `ConRender(const ConRender&) = delete;` и т.п.
3. **Деструктор не должен трогать то, что не создавалось** — например, клиентский
   сокет у объекта, который был сервером или так и не подключился.
4. **Неинициализированных переменных нет.**
5. **Строка пользователя не передаётся как форматная строка** в `printw`-подобные
   функции: `mvprintw(y, x, "%s", str.c_str())` или `mvaddstr`.
6. **Ввод в режиме ncurses — через `getch()`**, а не `getchar()`.
7. **Сравнение знаковых и беззнаковых** — использовать `size_t` для индексов
   по `.size()`.
8. **Ошибки сети** (отказ в подключении, занятый порт) обрабатываются, а не роняют
   программу: исключения Boost.Asio ловятся или используется перегрузка
   с `boost::system::error_code`.
9. Устаревшее API заменять на актуальное, если старое не собирается
   с современным Boost: `io_service` → `io_context`,
   `address::from_string` → `make_address`.
