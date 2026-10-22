// The main menu: 10 player slots and NEW GAME / REPLAY LEVEL / DELETE PLAYER / OPTIONS / EXIT,
// with the prompts and messages of the original menu.
#pragma once

#include <functional>
#include <string>

#include <vs/engine/ui_text_field.hpp>
#include <vs/engine/ui_theme.hpp>

#include "base_scene.hpp"
#include "options_panel.hpp"
#include "ui_kit.hpp"

namespace sg {

class MenuScene : public BaseScene {
public:
    explicit MenuScene(ResourcesPtr res);
    void on_enter() override;
    void on_exit() override;

protected:
    void tick(double dt) override;
    void draw(vs::Renderer& r) override;

private:
    enum Item { NewGame, ReplayLevel, DeletePlayer, Options, Exit, ItemCount };
    enum class Focus { Players, Items };

    vs::Theme field_theme_;
    vs::TextField field_{&field_theme_};
    bool prompt_open_ = false;
    std::string prompt_title_, prompt_hint_;
    NoticeBox notice_;
    ConfirmBox confirm_;
    OptionsPanel options_;
    Focus focus_ = Focus::Items;
    int item_ = 0;
    bool first_enter_ = true;

    int slot() const { return res().save.selected; }
    void select_slot(int s);
    void activate(int item);

    void ask_name();
    void ask_level(bool replay);
    void open_prompt(std::string title, std::string hint, std::size_t max_len, bool digits, std::string initial,
                     std::function<void(const std::string&)> on_submit);
    void close_prompt();

    void start_game(int level);
    void start_replay(int level);
    void ask_exit();

    vs::RectF slot_rect(int i) const;
    vs::RectF item_rect(int i) const;
    void draw_players(vs::Renderer& r);
    void draw_items(vs::Renderer& r);
    void draw_prompt(vs::Renderer& r);
};

}  // namespace sg
