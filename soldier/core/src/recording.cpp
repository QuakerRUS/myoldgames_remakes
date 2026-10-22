#include "soldier/recording.hpp"

#include <algorithm>

#include <nlohmann/json.hpp>

namespace soldier {
namespace {

std::string_view status_name(Status s) {
    switch (s) {
    case Status::Playing: return "playing";
    case Status::Won: return "won";
    case Status::Dead: return "dead";
    }
    return "?";
}

Status parse_status(const std::string& s) {
    if (s == "won") return Status::Won;
    if (s == "dead") return Status::Dead;
    if (s == "playing") return Status::Playing;
    throw FormatError("unknown result status '" + s + "'");
}

DeathCause parse_cause(const std::string& s) {
    for (DeathCause c : {DeathCause::GaveUp, DeathCause::TimeOut, DeathCause::Guillotine, DeathCause::Drowned})
        if (s == death_cause_name(c)) return c;
    throw FormatError("unknown death cause '" + s + "'");
}

std::string quote(std::string_view s) { return nlohmann::json(std::string(s)).dump(); }

}  // namespace

Recording parse_recording_json(std::string_view text) {
    Recording r;
    try {
        const nlohmann::json doc = nlohmann::json::parse(text);
        if (doc.at("format").get<std::string>() != "soldier-recording") throw FormatError("not a soldier recording");
        if (doc.at("version").get<int>() != 1) throw FormatError("unsupported recording version");
        r.level = doc.at("level").get<int>();
        r.player = doc.at("player").get<std::string>();
        if (doc.contains("source")) r.source = doc["source"].get<std::string>();
        if (doc.contains("result") && !doc["result"].is_null()) {
            const auto& res = doc["result"];
            RecordedResult rr;
            rr.status = parse_status(res.at("status").get<std::string>());
            rr.tick = res.at("tick").get<int>();
            if (res.contains("cause")) rr.cause = parse_cause(res["cause"].get<std::string>());
            r.result = rr;
        }
        int last = 0;
        for (const auto& item : doc.at("commands")) {
            if (!item.is_array() || item.size() != 2) throw FormatError("a command must be [tick, name]");
            const int tick = item[0].get<int>();
            const auto name = item[1].get<std::string>();
            const auto cmd = command_from_name(name);
            if (!cmd) throw FormatError("unknown command '" + name + "'");
            if (tick < last) throw FormatError("command ticks must not decrease");
            last = tick;
            r.commands.push_back({tick, *cmd});
        }
    } catch (const nlohmann::json::exception& e) {
        throw FormatError(std::string("recording JSON: ") + e.what());
    }
    return r;
}

Recording load_recording_file(const std::string& path) { return parse_recording_json(read_text_file(path)); }

std::string recording_to_json(const Recording& r) {
    std::string out = "{\n  \"format\": \"soldier-recording\",\n  \"version\": 1,\n";
    out += "  \"level\": " + std::to_string(r.level) + ",\n";
    out += "  \"player\": " + quote(r.player) + ",\n";
    if (!r.source.empty()) out += "  \"source\": " + quote(r.source) + ",\n";
    if (r.result) {
        out += "  \"result\": {\"status\": " + quote(status_name(r.result->status)) +
               ", \"tick\": " + std::to_string(r.result->tick);
        if (r.result->cause) out += ", \"cause\": " + quote(death_cause_name(*r.result->cause));
        out += "},\n";
    }
    out += "  \"commands\": [";
    for (std::size_t i = 0; i < r.commands.size(); ++i) {
        out += (i % 8 == 0) ? "\n    " : " ";
        out += "[" + std::to_string(r.commands[i].tick) + ", " + quote(command_name(r.commands[i].command)) + "]";
        if (i + 1 < r.commands.size()) out += ",";
    }
    out += r.commands.empty() ? "]\n}\n" : "\n  ]\n}\n";
    return out;
}

Recording make_recording(const Game& game, int level_number, std::string player) {
    Recording r;
    r.level = level_number;
    r.player = std::move(player);
    r.commands = game.history();
    if (game.status() != Status::Playing) {
        RecordedResult rr{game.status(), game.tick_count(), std::nullopt};
        if (game.status() == Status::Dead) rr.cause = game.death_cause();
        r.result = rr;
    }
    return r;
}

Replayer::Replayer(Level level, Recording recording) : game_(std::move(level)), rec_(std::move(recording)) {
    apply_due();
}

void Replayer::apply_due() {
    while (next_ < rec_.commands.size() && rec_.commands[next_].tick <= game_.tick_count() &&
           game_.status() == Status::Playing)
        game_.apply(rec_.commands[next_++].command);
}

void Replayer::step() {
    game_.tick();
    apply_due();
}

ReplayOutcome run_replay(const Level& level, const Recording& rec, int extra_ticks) {
    Replayer r(level, rec);
    const int last = rec.commands.empty() ? 0 : rec.commands.back().tick;
    while (!r.finished() && r.game().tick_count() < last + extra_ticks) r.step();
    ReplayOutcome o;
    o.status = r.game().status();
    o.cause = r.game().death_cause();
    o.tick = r.game().tick_count();
    o.unused_commands = rec.commands.size() - r.applied();
    const auto& h = r.game().history();
    o.in_sync = h.size() == r.applied() && std::equal(h.begin(), h.end(), rec.commands.begin());
    return o;
}

}  // namespace soldier
