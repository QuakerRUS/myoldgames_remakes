#include "dialogs.hpp"

#include <utility>

namespace ks {

namespace {
Block heading(const char* t) { return {Block::Kind::Heading, t}; }
Block text(const char* t) { return {Block::Kind::Text, t}; }
}  // namespace

Modal::Spec help_dialog(std::function<void()> on_close) {
    Modal::Spec s;
    s.title = "Помощь";
    s.width = 780;
    s.blocks = {
        text("Слепая десятипальцевая печать — набор текста, не глядя на клавиатуру. "
             "За каждым пальцем закреплены свои клавиши, поэтому руки почти не двигаются, "
             "а скорость растёт с каждым занятием."),
        heading("Исходная позиция"),
        text("Пальцы левой руки лежат на клавишах Ф Ы В А, правой — на О Л Д Ж, большие пальцы — "
             "на пробеле. На клавишах А и О есть выступы: находите их на ощупь. "
             "После каждого нажатия возвращайте палец на место."),
        heading("Цвета клавиш"),
        text("Цвет клавиши показывает палец, которым её нажимают; цифры под клавиатурой — номера "
             "пальцев (1 — большой, 5 — мизинец). Заглавные буквы и верхние символы набирайте "
             "с Shift другой руки."),
        heading("Как работает тренажёр"),
        text("Печатайте текст из верхнего окна. Верная клавиша сдвигает курсор, ошибочная "
             "вспыхивает красным и считается ошибкой. В конце строки нажмите пробел. Законченное "
             "предложение уходит вверх, внизу появляется новое. Нужная клавиша и палец подсвечены. "
             "Время идёт с первого нажатия, скорость — в знаках в минуту."),
        heading("Клавиши"),
        text("F1 — помощь,  F2 — рестарт,  F3 или Pause — пауза,  F4 — о программе,  F5 — звук,  "
             "Esc — закрыть окно или выйти. Включите русскую раскладку клавиатуры."),
    };
    s.buttons = {{"Понятно", std::move(on_close), true}};
    return s;
}

Modal::Spec about_dialog(const vs::Texture* icon, std::function<void()> on_close) {
    Modal::Spec s;
    s.width = 520;
    s.center_buttons = true;
    s.blocks = {
        {Block::Kind::Image, "", 104, icon},
        {Block::Kind::CenterTitle, "Keyboard Simulator"},
        {Block::Kind::Center, "Клавиатурный тренажёр слепой печати"},
        {Block::Kind::CenterMuted, "Версия 2.0 — ремейк дипломной программы 2007 года"},
        {Block::Kind::Divider},
        {Block::Kind::Center, "Program by Fedoseyenko Victor"},
        {Block::Kind::Space, "", 6},
        {Block::Kind::Center, "V Software Entertainment"},
        {Block::Kind::CenterMuted, "Russia, Moscow, 2026"},
    };
    s.buttons = {{"Закрыть", std::move(on_close), true}};
    return s;
}

Modal::Spec quit_dialog(std::function<void()> on_quit, std::function<void()> on_stay) {
    Modal::Spec s;
    s.title = "Выйти из тренажёра?";
    s.width = 500;
    s.blocks = {text("Статистика текущего занятия не сохраняется.")};
    s.buttons = {{"Остаться", std::move(on_stay), false}, {"Выйти", std::move(on_quit), true}};
    s.cancel = 0;
    s.selected = 1;
    return s;
}

}  // namespace ks
