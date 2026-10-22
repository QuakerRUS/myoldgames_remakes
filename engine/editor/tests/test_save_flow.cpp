#include <doctest/doctest.h>
#include <vs/editor/save_flow.hpp>

#include <vector>

TEST_CASE("save flow: save, save as, confirm and quit") {
    vs::ConfirmPrompt prompt;
    bool modified = false;
    std::filesystem::path path;
    std::filesystem::path chosen = "chosen.json";
    bool allow = true;
    std::vector<std::filesystem::path> written;
    std::vector<vs::AfterSave> ran;
    vs::SaveFlow::Hooks h;
    h.modified = [&] { return modified; };
    h.path = [&] { return path; };
    h.choose_path = [&](vs::SaveFlow::PathCallback done) { done(chosen); };
    h.confirm_write = [&](const std::filesystem::path&, std::function<void(bool)> proceed) { proceed(allow); };
    h.write = [&](const std::filesystem::path& p) {
        written.push_back(p);
        path = p;
        modified = false;
        return true;
    };
    h.run = [&](vs::AfterSave a) { ran.push_back(a); };
    vs::SaveFlow flow(prompt, h);

    // Unmodified: actions run at once, quitting is allowed.
    flow.ask_save_then(vs::AfterSave::Open);
    REQUIRE(ran.size() == 1);
    CHECK(ran[0] == vs::AfterSave::Open);
    CHECK(flow.request_quit());

    // Untitled Save goes through Save As; titled Save writes to the path.
    modified = true;
    flow.save();
    REQUIRE(written.size() == 1);
    CHECK(written[0] == "chosen.json");
    modified = true;
    flow.save();
    CHECK(written.size() == 2);

    // A refused confirmation writes nothing; a cancelled dialog writes nothing.
    modified = true;
    allow = false;
    flow.save_as();
    CHECK(written.size() == 2);
    allow = true;
    chosen.clear();
    flow.save_as();
    CHECK(written.size() == 2);
    CHECK(flow.pending() == vs::AfterSave::Nothing);

    // Modified: quitting asks first.
    CHECK_FALSE(flow.request_quit());
    CHECK(prompt.is_open());
}
