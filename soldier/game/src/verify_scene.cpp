#include "verify_scene.hpp"

#include <cstdio>

#include "levels.hpp"

namespace sg {

void VerifyScene::on_enter() {
    std::optional<soldier::Recording> rec;
    int n = 0;
    while (!rec && next_ <= kLevelCount) {
        n = next_++;
        rec = builtin_record(n);
        if (!rec) {
            std::printf("level %02d: NO RECORD FAIL\n", n);
            std::fflush(stdout);
            ++failures_;
        }
    }
    if (!rec) {
        finish();
        return;
    }
    auto scene = std::make_unique<ReplayScene>(shared_res(), load_level(n), *rec);
    scene->set_verify([this](const ReplayResult& r) {
        const bool ok = r.status == soldier::Status::Won && (r.expected_tick < 0 || r.expected_tick == r.tick);
        if (!ok) ++failures_;
        std::printf("level %02d: %s at tick %d (recorded %d) %s\n", r.level,
                    r.status == soldier::Status::Won    ? "won"
                    : r.status == soldier::Status::Dead ? "DIED"
                                                        : "UNFINISHED",
                    r.tick, r.expected_tick, ok ? "OK" : "FAIL");
        std::fflush(stdout);
        results_.push_back(r);
        app().pop_scene();
    });
    app().push_scene(std::move(scene));
}

void VerifyScene::finish() {
    std::printf("%d of %d records end in a win; %d failure(s)\n", kLevelCount - failures_, kLevelCount, failures_);
    std::fflush(stdout);
    app().quit(failures_ == 0 ? 0 : 1);
}

void VerifyScene::draw(vs::Renderer& r) { r.clear(vs::rgb(0x000000)); }

}  // namespace sg
