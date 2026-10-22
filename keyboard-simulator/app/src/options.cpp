#include "options.hpp"

#include <cstdlib>

namespace ks {

Options parse_options(const std::vector<std::string>& args) {
    Options o;
    for (std::size_t i = 0; i + 1 < args.size(); ++i) {
        const std::string& a = args[i];
        const std::string& v = args[i + 1];
        if (a == "--screenshot") o.screenshot = v;
        else if (a == "--autotype") o.autotype = std::atoi(v.c_str());
        else if (a == "--type") o.type = v;
        else if (a == "--state") o.state = v;
        else if (a == "--seed") o.seed = std::strtoull(v.c_str(), nullptr, 10);
        else continue;
        ++i;
    }
    return o;
}

}  // namespace ks
