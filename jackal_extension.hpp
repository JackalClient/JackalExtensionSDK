#pragma once
#include "jackal_extension.h"
#include <nlohmann/json.hpp>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace jackal {
class Host {
    const JcExtensionApiV1* api_ = nullptr;
    void* context_ = nullptr;
public:
    bool initialize(const JcExtensionHostV1* host) {
        if (!host || host->version != JC_EXTENSION_ABI_VERSION || host->size < offsetof(JcExtensionHostV1, context) + sizeof(void*)
            || !host->api || host->api->size < sizeof(JcExtensionApiV1) || host->api->version != JC_EXTENSION_API_VERSION
            || !host->api->query || !host->api->post || !host->context) return false;
        api_ = host->api; context_ = host->context; return true;
    }
    nlohmann::json query(const nlohmann::json& request) const {
        if (!api_) throw std::runtime_error("Jackal host unavailable.");
        const auto text = request.dump();
        std::vector<char> buffer(4096);
        uint32_t required = 0;
        int status = api_->query(context_, text.c_str(), buffer.data(), static_cast<uint32_t>(buffer.size()), &required);
        if (status == JC_EXTENSION_BUFFER_TOO_SMALL && required && required <= 16 * 1024 * 1024) {
            buffer.resize(required);
            status = api_->query(context_, text.c_str(), buffer.data(), static_cast<uint32_t>(buffer.size()), &required);
        }
        if (status != JC_EXTENSION_OK) throw std::runtime_error(status == JC_EXTENSION_WRONG_THREAD ? "Query must run in a client callback." : "Jackal query failed.");
        return nlohmann::json::parse(buffer.data());
    }
    nlohmann::json get(const nlohmann::json& request) const {
        auto result = query(request);
        if (!result.value("ok", false)) throw std::runtime_error(result.value("error", "Jackal query rejected."));
        return result.at("value");
    }
    uint64_t post(const nlohmann::json& request) const {
        if (!api_) throw std::runtime_error("Jackal host unavailable.");
        const auto text = request.dump();
        uint64_t ticket = 0;
        if (api_->post(context_, text.c_str(), &ticket) != JC_EXTENSION_OK) throw std::runtime_error("Jackal request queue rejected the operation.");
        return ticket;
    }
    bool moduleEnabled(const std::string& module) const { return get({ { "method", "module.state" }, { "module", module } }).get<bool>(); }
    bool moduleExists(const std::string& module) const { return get({ { "method", "module.exists" }, { "module", module } }).get<bool>(); }
    std::vector<std::string> modules() const { return get({ { "method", "module.list" } }).get<std::vector<std::string>>(); }
    nlohmann::json info() const { return get({ { "method", "client.info" } }); }
    nlohmann::json permissions() const { return get({ { "method", "client.permissions" } }); }
    nlohmann::json paths() const { return get({ { "method", "client.paths" } }); }
    nlohmann::json state() const { return get({ { "method", "client.state" } }); }
    std::vector<std::string> variables() const { return get({ { "method", "variable.list" } }).get<std::vector<std::string>>(); }
    std::string variable(const std::string& name) const { return get({ { "method", "variable.get" }, { "name", name } }).get<std::string>(); }
    std::string translate(const std::string& text) const { return get({ { "method", "text.translate" }, { "text", text } }).get<std::string>(); }
    std::string expand(const std::string& text) const { return get({ { "method", "text.expand" }, { "text", text } }).get<std::string>(); }
    bool keyDown(const std::vector<int>& keys, bool clientOnly = false) const { return get({ { "method", "input.keyDown" }, { "keys", keys }, { "clientOnly", clientOnly } }).get<bool>(); }
    bool keyDown(int key, bool clientOnly = false) const { return keyDown(std::vector<int>{key}, clientOnly); }
    nlohmann::json privateWindows() const { return get({ { "method", "window.privateList" } }); }
    nlohmann::json foregroundWindow() const { return get({ { "method", "window.foreground" } }); }
    nlohmann::json hudElements() const { return get({ { "method", "customhud.list" } }); }
    uint64_t addHudElement(const nlohmann::json& element) const { return post({ { "method", "customhud.add" }, { "element", element } }); }
    uint64_t setHudElement(size_t index, const nlohmann::json& element) const { return post({ { "method", "customhud.set" }, { "index", index }, { "element", element } }); }
    uint64_t updateHudElement(size_t index, const nlohmann::json& patch) const { return post({ { "method", "customhud.update" }, { "index", index }, { "patch", patch } }); }
    uint64_t removeHudElement(size_t index) const { return post({ { "method", "customhud.remove" }, { "index", index } }); }
    uint64_t replaceHudElements(const nlohmann::json& elements) const { return post({ { "method", "customhud.replace" }, { "elements", elements } }); }
    uint64_t http(const std::string& url, nlohmann::json options = nlohmann::json::object()) const {
        options["method"] = "http.request"; options["url"] = url; return post(options);
    }
    nlohmann::json requestResult(uint64_t ticket) const { return query({ { "method", "request.result" }, { "ticket", ticket } }); }
    nlohmann::json config(const std::string& module) const { return get({ { "method", "config.get" }, { "module", module } }); }
    nlohmann::json config(const std::string& module, const std::string& key) const { return get({ { "method", "config.get" }, { "module", module }, { "key", key } }); }
    uint64_t setModule(const std::string& module, bool enabled) const { return post({ { "method", "module.set" }, { "module", module }, { "enabled", enabled } }); }
    uint64_t notify(const std::string& text, const std::string& level = "info", const std::string& title = "") const {
        nlohmann::json request { { "method", "notify" }, { "text", text }, { "level", level } };
        if (!title.empty()) request["title"] = title;
        return post(request);
    }
    uint64_t command(const std::string& command) const { return post({ { "method", "command.execute" }, { "command", command } }); }
};
}
