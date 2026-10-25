// Touch tracking of the input manager (the TS touch contact pool) and TouchCollection, driven through `report_touch`:
// no SDL event, no device.
#include <catch2/catch_test_macros.hpp>

#include <ris/input.hpp>

using ris::TouchLocationState;
using ris::TouchPhase;
using ris::InputManager;

TEST_CASE("touch capabilities start disconnected and follow the first contact") {
    InputManager input;
    REQUIRE_FALSE(input.touch_capabilities().is_connected());
    REQUIRE(input.touch_collection().count() == 0);
    input.report_touch(TouchPhase::Begin, 1, {10, 20}, 0.0f);
    REQUIRE(input.touch_capabilities().is_connected());
    REQUIRE(input.touch_capabilities().maximum_touch_count() >= 1);
}

TEST_CASE("no touch device: the collection stays empty") {
    InputManager input;
    input.update();
    input.after_update();
    input.update();
    REQUIRE(input.touch_collection().count() == 0);
    REQUIRE(input.touch_collection().get(0) == nullptr);
    REQUIRE_FALSE(input.touch_collection().is_down(1));
}

TEST_CASE("a touch is pressed, then moved, then released, then gone") {
    InputManager input;
    input.report_touch(TouchPhase::Begin, 7, {10, 20}, 0.5f);
    input.update();
    {
        const ris::TouchLocation* t = input.touch_collection().find_by_id(7);
        REQUIRE(t != nullptr);
        REQUIRE(t->state() == TouchLocationState::Pressed);
        REQUIRE(t->is_pressed());
        REQUIRE(t->is_down());
        REQUIRE(t->x() == 10);
        REQUIRE(t->y() == 20);
        REQUIRE(t->pressure == 0.5f);
        REQUIRE(t->try_get_previous_location() == nullptr);
        REQUIRE(input.touch_collection().active_count() == 1);
    }
    input.after_update();

    input.report_touch(TouchPhase::Move, 7, {15, 26}, 0.5f);
    input.update();
    {
        const ris::TouchLocation* t = input.touch_collection().find_by_id(7);
        REQUIRE(t != nullptr);
        REQUIRE(t->state() == TouchLocationState::Moved);
        REQUIRE(t->x() == 15);
        const ris::TouchLocation* previous = t->try_get_previous_location();
        REQUIRE(previous != nullptr);
        REQUIRE(previous->x() == 10);
        REQUIRE(previous->y() == 20);
        REQUIRE(previous->state() == TouchLocationState::Pressed);
        REQUIRE(t->dx() == 5);
        REQUIRE(t->dy() == 6);
    }
    input.after_update();

    input.report_touch(TouchPhase::End, 7, {15, 26}, 0.0f);
    input.update();
    {
        const ris::TouchLocation* t = input.touch_collection().find_by_id(7);
        REQUIRE(t != nullptr);
        REQUIRE(t->is_released());
        REQUIRE_FALSE(t->is_down());
        REQUIRE(t->pressure == 0.5f);   // the end report has no pressure, the last real one is kept
        REQUIRE(input.touch_collection().is_released(7));
        REQUIRE_FALSE(input.touch_collection().is_cancelled(7));
        REQUIRE(input.touch_collection().active_count() == 0);
    }
    input.after_update();

    input.update();   // the released contact is gone
    REQUIRE(input.touch_collection().find_by_id(7) == nullptr);
    REQUIRE(input.touch_collection().count() == 0);
}

TEST_CASE("the published touches stay readable until the next update") {
    InputManager input;
    input.report_touch(TouchPhase::Begin, 1, {1, 1}, 0);
    input.update();
    input.report_touch(TouchPhase::Move, 1, {9, 9}, 0);   // collected after the publish
    REQUIRE(input.touch_collection().find_by_id(1)->x() == 1);
    input.after_update();
    REQUIRE(input.touch_collection().find_by_id(1)->x() == 1);
    input.update();
    REQUIRE(input.touch_collection().find_by_id(1)->x() == 9);
}

TEST_CASE("a press that ends in the same frame is pressed once, then released") {
    InputManager input;
    input.report_touch(TouchPhase::Begin, 3, {4, 4}, 0);
    input.report_touch(TouchPhase::End, 3, {4, 4}, 0);
    input.update();
    REQUIRE(input.touch_collection().find_by_id(3)->is_pressed());
    input.after_update();
    input.update();
    REQUIRE(input.touch_collection().find_by_id(3)->is_released());
    input.after_update();
    input.update();
    REQUIRE(input.touch_collection().find_by_id(3) == nullptr);
}

TEST_CASE("a cancelled touch is not a lift") {
    InputManager input;
    input.report_touch(TouchPhase::Begin, 5, {2, 2}, 0);
    input.update();
    input.after_update();
    input.report_touch(TouchPhase::Cancel, 5, {2, 2}, 0);
    input.update();
    const ris::TouchLocation* t = input.touch_collection().find_by_id(5);
    REQUIRE(t != nullptr);
    REQUIRE(t->is_cancelled());
    REQUIRE_FALSE(t->is_released());
    REQUIRE(input.touch_collection().is_cancelled(5));
    REQUIRE_FALSE(input.touch_collection().is_released(5));
    input.after_update();
    input.update();
    REQUIRE(input.touch_collection().find_by_id(5) == nullptr);
}

TEST_CASE("several touches get their own contacts and ids can be reused after the release") {
    InputManager input;
    input.report_touch(TouchPhase::Begin, 1, {1, 0}, 0);
    input.report_touch(TouchPhase::Begin, 2, {2, 0}, 0);
    input.update();
    REQUIRE(input.touch_collection().count() == 2);
    REQUIRE(input.touch_collection().active_count() == 2);
    REQUIRE(input.touch_collection().is_down(1));
    REQUIRE(input.touch_collection().is_down(2));
    input.after_update();

    input.report_touch(TouchPhase::End, 1, {1, 0}, 0);
    input.update();
    input.after_update();
    input.update();
    REQUIRE(input.touch_collection().count() == 1);
    REQUIRE(input.touch_collection().is_down(2));

    input.report_touch(TouchPhase::Begin, 1, {8, 8}, 0);
    input.update();
    REQUIRE(input.touch_collection().find_by_id(1)->is_pressed());
    REQUIRE(input.touch_collection().find_by_id(1)->x() == 8);
}

TEST_CASE("more contacts than the pool holds are ignored") {
    InputManager input;
    for (int32_t id = 1; id <= ris::TouchCollection::default_max_touch_count + 4; ++id) {
        input.report_touch(TouchPhase::Begin, id, {static_cast<float>(id), 0}, 0);
    }
    input.update();
    REQUIRE(input.touch_collection().count() <= ris::TouchCollection::default_max_touch_count);
    REQUIRE(input.touch_collection().count() >= 1);
}

TEST_CASE("moves and ends of unknown touches are ignored") {
    InputManager input;
    input.report_touch(TouchPhase::Move, 42, {1, 1}, 0);
    input.report_touch(TouchPhase::End, 42, {1, 1}, 0);
    input.report_touch(TouchPhase::Cancel, 42, {1, 1}, 0);
    input.update();
    REQUIRE(input.touch_collection().count() == 0);
}

TEST_CASE("TouchCollection stores locations up to its capacity") {
    ris::TouchCollection collection(2);
    REQUIRE(collection.max_touch_count() == 2);
    REQUIRE(collection.try_add(1, TouchLocationState::Pressed, 1, 2, 0.25f) != nullptr);
    REQUIRE(collection.try_add(2, TouchLocationState::Released, 3, 4) != nullptr);
    REQUIRE(collection.try_add(3, TouchLocationState::Pressed, 5, 6) == nullptr);   // full
    REQUIRE(collection.count() == 2);
    REQUIRE(collection.active_count() == 1);   // a released touch is not down
    REQUIRE(collection.get(1)->id == 2);
    REQUIRE(collection.get(2) == nullptr);
    REQUIRE(collection.get(-1) == nullptr);
    collection.clear();
    REQUIRE(collection.count() == 0);
}

TEST_CASE("TouchCollection with a previous sample") {
    ris::TouchCollection collection;
    const ris::TouchLocation* t = collection.try_add(9, TouchLocationState::Moved, 5, 5, 1, TouchLocationState::Pressed, 2, 3, 1);
    REQUIRE(t != nullptr);
    const ris::TouchLocation* previous = t->try_get_previous_location();
    REQUIRE(previous != nullptr);
    REQUIRE(previous->x() == 2);
    REQUIRE(t->dx() == 3);
    REQUIRE(t->dy() == 2);
}
