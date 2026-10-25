#include <catch2/catch_test_macros.hpp>

#include <string>
#include <thread>
#include <vector>

#include "ris/utilities/listener_id_generator.hpp"
#include "ris/utilities/listener_map.hpp"

TEST_CASE("listener id generator gives unique, increasing, non zero ids") {
    ris::ListenerIdGenerator generator;
    REQUIRE(generator.last() == 0);
    ris::ListenerId previous = 0;
    for (int i = 0; i < 100; ++i) {
        const ris::ListenerId id = generator.next();
        REQUIRE(id != 0);
        REQUIRE(id > previous);
        previous = id;
    }
    REQUIRE(generator.last() == previous);
    REQUIRE(previous == 100);
}

TEST_CASE("listener id generator is unique across threads") {
    ris::ListenerIdGenerator generator;
    std::vector<std::vector<ris::ListenerId>> ids(4);
    std::vector<std::thread> threads;
    for (auto& v : ids) threads.emplace_back([&] { for (int i = 0; i < 1000; ++i) v.push_back(generator.next()); });
    for (auto& t : threads) t.join();
    std::vector<bool> seen(4001, false);
    for (const auto& v : ids) for (const ris::ListenerId id : v) {
        REQUIRE(id >= 1);
        REQUIRE(id <= 4000);
        REQUIRE_FALSE(seen[id]);
        seen[id] = true;
    }
}

TEST_CASE("listener map calls listeners in id order and passes the arguments") {
    ris::ListenerIdGenerator generator;
    ris::ListenerMap<void(int, const std::string&)> map(generator);
    REQUIRE(map.empty());

    std::vector<std::string> calls;
    const auto a = map.add([&](int n, const std::string& s) { calls.push_back("a" + std::to_string(n) + s); });
    const auto b = map.add([&](int n, const std::string& s) { calls.push_back("b" + std::to_string(n) + s); });
    const auto c = map.add([&](int n, const std::string& s) { calls.push_back("c" + std::to_string(n) + s); });
    REQUIRE(a < b);
    REQUIRE(b < c);
    REQUIRE(map.size() == 3);

    map.invoke(7, "x");
    REQUIRE(calls == std::vector<std::string>{"a7x", "b7x", "c7x"});
}

TEST_CASE("listener map remove erases the listener and reports unknown ids") {
    ris::ListenerIdGenerator generator;
    ris::ListenerMap<void()> map(generator);
    int a = 0, b = 0;
    const auto ida = map.add([&] { ++a; });
    const auto idb = map.add([&] { ++b; });

    REQUIRE(map.remove(ida));
    REQUIRE_FALSE(map.remove(ida));   // already removed
    REQUIRE_FALSE(map.remove(0));
    REQUIRE_FALSE(map.remove(idb + 100));
    map.invoke();
    REQUIRE(a == 0);
    REQUIRE(b == 1);
    REQUIRE(map.size() == 1);

    map.clear();
    REQUIRE(map.empty());
    map.invoke();   // nothing to call
    REQUIRE(b == 1);
}

TEST_CASE("listener map: a listener removed during invoke is not called, even when it comes later") {
    ris::ListenerIdGenerator generator;
    ris::ListenerMap<void()> map(generator);
    std::vector<int> calls;
    ris::ListenerId second = 0;
    map.add([&] { calls.push_back(1); map.remove(second); });
    second = map.add([&] { calls.push_back(2); });
    map.add([&] { calls.push_back(3); });

    map.invoke();
    REQUIRE(calls == std::vector<int>{1, 3});
    map.invoke();
    REQUIRE(calls == std::vector<int>{1, 3, 1, 3});
}

TEST_CASE("listener map: a listener can remove itself") {
    ris::ListenerIdGenerator generator;
    ris::ListenerMap<void()> map(generator);
    int once = 0, always = 0;
    ris::ListenerId self = 0;
    self = map.add([&, payload = std::string("kept alive while running")] {
        REQUIRE(map.remove(self));
        ++once;
        REQUIRE(payload.size() > 5);   // the callable must survive its own removal
    });
    map.add([&] { ++always; });

    map.invoke();
    map.invoke();
    REQUIRE(once == 1);
    REQUIRE(always == 2);
}

TEST_CASE("listener map: a listener added during invoke first runs on the next invoke") {
    ris::ListenerIdGenerator generator;
    ris::ListenerMap<void()> map(generator);
    int added_calls = 0;
    bool added = false;
    map.add([&] {
        if (added) return;
        added = true;
        map.add([&] { ++added_calls; });
    });

    map.invoke();
    REQUIRE(added_calls == 0);
    map.invoke();
    REQUIRE(added_calls == 1);
}

TEST_CASE("listener maps sharing one static style generator keep ids unique and ordered across maps") {
    ris::ListenerIdGenerator shared;
    ris::ListenerMap<void()> first(shared), second(shared);
    std::vector<ris::ListenerId> ids;
    ids.push_back(first.add([] {}));
    ids.push_back(second.add([] {}));
    ids.push_back(first.add([] {}));
    for (size_t i = 1; i < ids.size(); ++i) REQUIRE(ids[i] > ids[i - 1]);
    REQUIRE_FALSE(second.remove(ids[0]));   // an id belongs to the map it was added to
    REQUIRE(first.remove(ids[0]));
}
