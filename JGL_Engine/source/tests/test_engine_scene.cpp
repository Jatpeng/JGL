#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <typeindex>
#include <unordered_map>
#include "engine/engine.h"
#include "engine/scene.h"
#include "window/window_overlay.h"

// Simple testing macro that works in Release builds too
#define EXPECT_TRUE(cond) \
    if (!(cond)) { \
        std::cerr << "FAILED: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return 1; \
    }

#define EXPECT_EQ(val1, val2) \
    if ((val1) != (val2)) { \
        std::cerr << "FAILED: " << #val1 << " == " << #val2 << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return 1; \
    }

class MockOverlay : public nwindow::IWindowOverlay {
public:
    void render(nengine::RenderEngine& engine) override { (void)engine; }
};

int main() {
    nengine::Engine::CreateInfo info;
    info.create_default_scene = false; // Disable default scene for testing
    nengine::Engine engine(info);

    // 1. Test create_scene
    auto scene1 = engine.create_scene("Scene1");
    EXPECT_TRUE(scene1 != nullptr);
    EXPECT_EQ(scene1->name(), "Scene1");
    std::cout << "Test 1 Passed: create_scene(Scene1) successful.\n";

    auto scene2 = engine.create_scene("Scene2");
    EXPECT_TRUE(scene2 != nullptr);
    EXPECT_EQ(scene2->name(), "Scene2");
    std::cout << "Test 2 Passed: create_scene(Scene2) successful.\n";

    // 2. Test active_scene (initially nullptr since create_default_scene=false)
    EXPECT_TRUE(engine.active_scene() == nullptr);
    std::cout << "Test 3 Passed: active_scene is initially nullptr.\n";

    // 3. Test set_active_scene
    engine.set_active_scene(scene1);
    EXPECT_EQ(engine.active_scene(), scene1);
    EXPECT_EQ(engine.active_scene()->name(), "Scene1");
    std::cout << "Test 4 Passed: set_active_scene(scene1) successful.\n";

    engine.set_active_scene(scene2);
    EXPECT_EQ(engine.active_scene(), scene2);
    EXPECT_EQ(engine.active_scene()->name(), "Scene2");
    std::cout << "Test 5 Passed: set_active_scene(scene2) successful.\n";

    // 4. Test switching back
    engine.set_active_scene(scene1);
    EXPECT_EQ(engine.active_scene(), scene1);
    std::cout << "Test 6 Passed: switching back to scene1 successful.\n";

    // 5. Test set_window_overlay
    auto mock_overlay = std::make_shared<MockOverlay>();
    engine.set_window_overlay(mock_overlay);
    std::cout << "Test 7 Passed: set_window_overlay successful.\n";

    std::cout << "All Engine Scene Management tests passed!\n";
    return 0;
}
