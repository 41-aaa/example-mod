// ReSharper disable CppHidingFunction
#include "JoystickNode.hpp"
#include "Geode/loader/Log.hpp"
#include "Geode/ui/Popup.hpp"
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/UILayer.hpp>
#include <Geode/modify/LevelSettingsLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <fmt/format.h>

#ifdef GEODE_IS_DESKTOP
#include <geode.custom-keybinds/include/Keybinds.hpp>

$on_game(Loaded) {
    auto mgr = keybinds::BindManager::get();
    mgr->registerBindable({
        "joystick_up"_spr,
        "Joystick Up",
        "Up for joystick",
        {
            keybinds::Keybind::create(KEY_W, keybinds::Modifier::None),
            keybinds::ControllerBind::create(CONTROLLER_LTHUMBSTICK_UP),
            keybinds::Keybind::create(KEY_ArrowUp, keybinds::Modifier::None),
        },
        "Joystick",
        false
    });
    mgr->registerBindable({
        "joystick_down"_spr,
        "Joystick Down",
        "Down for joystick",
        {
            keybinds::Keybind::create(KEY_S, keybinds::Modifier::None),
            keybinds::ControllerBind::create(CONTROLLER_LTHUMBSTICK_DOWN),
            keybinds::Keybind::create(KEY_ArrowDown, keybinds::Modifier::None),
        },
        "Joystick",
        false
    });
}
#endif

bool enableJoystick = false;

void runChecks(CCArray *objects) {
    enableJoystick = false;
    for (auto obj : CCArrayExt<GameObject*>(objects)) {
        if (obj->m_objectID == 914) {
            if (auto txt = static_cast<TextGameObject*>(obj)) {
                if (txt->m_text == "--enable-joystick") 
                    enableJoystick = true;
            } 
        }
    }
}

// Updated to take float and scale for 3 decimal precision in item counters
void updateVal(GJBaseGameLayer *layer, int id, float val) {
    if (enableJoystick) {
        int scaledVal = static_cast<int>(val * 1000.0f);
        layer->m_effectManager->updateCountForItem(id, scaledVal);
        layer->updateCounters(id, scaledVal);
    }
}

bool JoystickNode::init() {
    if (!CCMenu::init()) return false;
    setContentSize({100, 100});

    m_bg = CCSprite::createWithSpriteFrameName("d_circle_02_001.png");
    m_bg->setScale(getContentSize().width / m_bg->getContentSize().width);
    m_bg->setOpacity(165);
    m_bg->setColor({189, 189, 189});
    addChildAtPosition(m_bg, Anchor::Center);

    m_center = CCSprite::createWithSpriteFrameName("d_circle_02_001.png");
    m_center->setScale(getContentSize().width / m_center->getContentSize().width / 3);
    m_center->setOpacity(236);
    m_center->setZOrder(1);
    m_center->setColor({0, 0, 0});
    addChildAtPosition(m_center, Anchor::Center);

    // Added a label to show the 3 decimal precision on the UI
    m_valueLabel = CCLabelBMFont::create("X: 0.000 Y: 0.000", "goldFont.fnt");
    m_valueLabel->setScale(0.4f);
    addChildAtPosition(m_valueLabel, Anchor::Top, {0, 15});

    registerWithTouchDispatcher();

    return true;
}

JoystickNode *JoystickNode::create() {
    auto ret = new JoystickNode;
    if (!ret->init()) {
        delete ret;
        return nullptr;
    }
    ret->autorelease();
    return ret;
}

bool JoystickNode::ccTouchBegan(CCTouch *touch, CCEvent *event) {
    if (!isTouchEnabled() || !nodeIsVisible(this)) return false;
    if (ccpDistance(getPosition(), touch->getLocation()) <= getScaledContentSize().width / 2) {
        ccTouchMoved(touch, event);
        return true;
    }
    return false;
}

void JoystickNode::handleInput(GJBaseGameLayer *layer, CCPoint input, CCPoint old) {
    // Platformer compatibility: Use a threshold of 0.5 for digital move buttons
    if (std::abs(input.x - old.x) > 0.001f) {
        if (old.x >= 0.5f) layer->queueButton(3, false, false); 
        else if (old.x <= -0.5f) layer->queueButton(2, false, false);
        
        if (input.x >= 0.5f) layer->queueButton(3, true, false);
        else if (input.x <= -0.5f) layer->queueButton(2, true, false);
    }

    if (layer->m_player1 && !layer->m_player1->m_controlsDisabled) {
        updateVal(layer, 3741, input.x);
        updateVal(layer, 3742, input.y);
    }

    // Update the visual text label
    m_valueLabel->setString(fmt::format("X: {:.3f} Y: {:.3f}", input.x, input.y).c_str());
}

void JoystickNode::fakePosition() {
    // Used for keyboard/controller input to sync the visual knob
    CCPoint pos = (getContentSize() / 2) + (m_currentInput * (getContentWidth() / 2));
    m_center->setPosition(pos);
    m_valueLabel->setString(fmt::format("X: {:.3f} Y: {:.3f}", m_currentInput.x, m_currentInput.y).c_str());
}

void JoystickNode::ccTouchEnded(CCTouch *touch, CCEvent *event) {
    if (auto uil = UILayer::get(); uil && uil->m_gameLayer) {
        handleInput(uil->m_gameLayer, {0, 0}, m_currentInput);
    }
    m_currentInput = CCPoint{0, 0};
    m_center->setPosition(getContentSize() / 2);
}

void JoystickNode::ccTouchMoved(CCTouch *touch, CCEvent *event) {
    auto pos = convertToNodeSpace(touch->getLocation());
    auto centerPos = getContentSize() / 2;
    auto fromCenter = pos - centerPos;
    float maxRadius = (getContentWidth() / 2) - (m_center->getContentWidth() * m_center->getScale() / 2);

    // Precise Analog Calculation
    if (fromCenter.getLength() > maxRadius) {
        fromCenter = fromCenter.normalize() * maxRadius;
    }

    // Normalized input between -1.000 and 1.000
    CCPoint inp = {
        fromCenter.x / maxRadius,
        fromCenter.y / maxRadius
    };

    if (auto uil = UILayer::get(); uil && uil->m_gameLayer) {
        handleInput(uil->m_gameLayer, inp, m_currentInput);
    }

    m_currentInput = inp;
    m_center->setPosition(centerPos + fromCenter);
}

void JoystickNode::ccTouchCancelled(CCTouch *touch, CCEvent *event) {
    ccTouchEnded(touch, event);
}

void JoystickNode::registerWithTouchDispatcher() {
    CCTouchDispatcher::get()->addTargetedDelegate(this, -512, true);
}

void JoystickNode::draw() {
    CCMenu::draw();
    // Debug visuals removed for brevity; can be re-added if required
}
#if defined(DEBUG_BUILD) && !defined(GITHUB_ACTIONS)
    {
        ccDrawColor4B(0, 255, 0, 100);
        auto center = getScaledContentSize() / 2;
        auto radius = getScaledContentWidth() / 2;
        auto startAngle = 5 * -M_PI / 12;
        auto sweepAngle = 5 * M_PI / 12 - 5 * -M_PI / 12;
        int segments = 50;

        std::vector<CCPoint> vertices;
        vertices.push_back(center);

        for (int i = 0; i <= segments; ++i) {
            float angle = startAngle + (sweepAngle * i / segments);
            vertices.push_back({
                center.width + radius * cos(angle),
                center.height + radius * sin(angle)
            });
        }

        ccDrawSolidPoly(vertices.data(), vertices.size(), ccc4f(0, 1, 0, 0.15f));
    }

    {
        ccDrawColor4B(0, 255, 0, 100);
        auto center = getScaledContentSize() / 2;
        auto radius = getScaledContentWidth() / 2;
        auto startAngle = 7 * M_PI / 12;
        auto sweepAngle = 11 * M_PI / 12 - M_PI / 12;
        int segments = 50;

        std::vector<CCPoint> vertices;
        vertices.push_back(center);

        for (int i = 0; i <= segments; ++i) {
            float angle = startAngle + (sweepAngle * i / segments);
            vertices.push_back({
                center.width + radius * cos(angle),
                center.height + radius * sin(angle)
            });
        }

        ccDrawSolidPoly(vertices.data(), vertices.size(), ccc4f(0, 1, 0, 0.15f));
    }

    {
        ccDrawColor4B(255, 0, 0, 100);
        auto center = getScaledContentSize() / 2;
        auto radius = getScaledContentWidth() / 2;
        auto startAngle = M_PI / 12;
        auto sweepAngle = 11 * M_PI / 12 - M_PI / 12;
        int segments = 50;

        std::vector<CCPoint> vertices;
        vertices.push_back(center);

        for (int i = 0; i <= segments; ++i) {
            float angle = startAngle + (sweepAngle * i / segments);
            vertices.push_back({
                center.width + radius * cos(angle),
                center.height + radius * sin(angle)
            });
        }

        ccDrawSolidPoly(vertices.data(), vertices.size(), ccc4f(1, 0, 0, 0.15f));
    }

    {
        ccDrawColor4B(255, 0, 0, 100);
        auto center = getScaledContentSize() / 2;
        auto radius = getScaledContentWidth() / 2;
        auto startAngle = -11 * M_PI / 12;
        auto sweepAngle = -M_PI / 12 - -11 * M_PI / 12;
        int segments = 50;

        std::vector<CCPoint> vertices;
        vertices.push_back(center);

        for (int i = 0; i <= segments; ++i) {
            float angle = startAngle + (sweepAngle * i / segments);
            vertices.push_back({
                center.width + radius * cos(angle),
                center.height + radius * sin(angle)
            });
        }

        ccDrawSolidPoly(vertices.data(), vertices.size(), ccc4f(1, 0, 0, 0.15f));
    }
    #endif
}

class $modify(JSUILayer, UILayer) {

    struct Fields {
        JoystickNode *m_joystickNode;
    };

    static void onModify(auto& self) {
        if (auto res = self.setHookPriorityBefore("UILayer::init", "geode.custom-keybinds"); res.isErr()) {
            geode::log::warn("Failed to set hook priority: {}", res.unwrapErr());
        }
    }

    bool init(GJBaseGameLayer *gjbgl) {
        if (!UILayer::init(gjbgl)) return false;
        queueInMainThread([this, gjbgl](){
            m_fields->m_joystickNode = JoystickNode::create();
            m_fields->m_joystickNode->m_twoPlayer = gjbgl->m_level->m_twoPlayerMode;
            addChildAtPosition(m_fields->m_joystickNode, Anchor::BottomLeft, {75, 75}, false);

            fixVisibility();
            #ifdef GEODE_IS_DESKTOP

            // mine
            m_gameLayer->addEventListener<keybinds::InvokeBindFilter>([this](keybinds::InvokeBindEvent* event) {
                if (!enableJoystick) return ListenerResult::Propagate;
                if (auto node = m_fields->m_joystickNode) {
                    auto old = node->m_currentInput;
                    if (event->isDown()) node->m_currentInput.y += 1;
                    else node->m_currentInput.y -= 1;
                    node->handleInput(m_gameLayer, node->m_currentInput, old);
                    node->fakePosition();
                }
                return ListenerResult::Stop;
            }, "joystick_up"_spr);
            m_gameLayer->addEventListener<keybinds::InvokeBindFilter>([this](keybinds::InvokeBindEvent* event) {
                if (!enableJoystick) return ListenerResult::Propagate;
                if (auto node = this->m_fields->m_joystickNode) {
                    auto old = node->m_currentInput;
                    if (event->isDown()) node->m_currentInput.y -= 1;
                    else node->m_currentInput.y += 1;
                    node->handleInput(this->m_gameLayer, node->m_currentInput, old);
                    node->fakePosition();
                }
                return ListenerResult::Stop;
            }, "joystick_down"_spr);

            m_gameLayer->addEventListener<keybinds::InvokeBindFilter>([this](keybinds::InvokeBindEvent* event) {
                if (!enableJoystick) return ListenerResult::Propagate;
                if (auto node = this->m_fields->m_joystickNode) {
                    auto old = node->m_currentInput;
                    if (event->isDown()) node->m_currentInput.x -= 1;
                    else node->m_currentInput.x += 1;
                    node->handleInput(this->m_gameLayer, node->m_currentInput, old);
                    node->fakePosition();
                }
                return ListenerResult::Stop;
            }, "robtop.geometry-dash/move-left-p1");
            m_gameLayer->addEventListener<keybinds::InvokeBindFilter>([this](keybinds::InvokeBindEvent* event) {
                if (!enableJoystick) return ListenerResult::Propagate;
                if (auto node = this->m_fields->m_joystickNode) {
                    auto old = node->m_currentInput;
                    if (event->isDown()) node->m_currentInput.x += 1;
                    else node->m_currentInput.x -= 1;
                    node->handleInput(this->m_gameLayer, node->m_currentInput, old);
                    node->fakePosition();
                }
                return ListenerResult::Stop;
            }, "robtop.geometry-dash/move-right-p1");
            #endif
        });
        return true;
    }

    void fixVisibility() {
        if (!m_fields->m_joystickNode) {
            return;
        }
        if (!enableJoystick || !m_inPlatformer) {
            m_fields->m_joystickNode->setVisible(false);
            m_fields->m_joystickNode->setTouchEnabled(false);
            return;
        }

        m_fields->m_joystickNode->setVisible(true);
        m_fields->m_joystickNode->setTouchEnabled(true);    

        if (auto p1move = getChildByID("platformer-p1-move-button")) {
            p1move->setPosition({10000, 10000});
        }
    }

    #if !defined(GEODE_IS_WINDOWS) && !defined(GEODE_IS_ARM_MAC)

    void refreshDpad() {
        UILayer::refreshDpad();
        
        fixVisibility();
    }

    #endif
};

class $modify(JSLEL, LevelEditorLayer) {
    struct Fields {
        bool m_hasVerified = false;
    };
#ifdef GEODE_IS_DESKTOP
    void popup(bool bounce = true) {
        auto jbinds = keybinds::BindManager::get()->getBindsFor("robtop.geometry-dash/jump-p1");
        auto mbinds = keybinds::BindManager::get()->getBindsFor("joystick_up"_spr);
        bool overlap = false;
        for (const auto& jbind : jbinds) {
            for (const auto& mbind : mbinds) {
                if (jbind->isEqual(mbind)) {
                    overlap = true;
                    break;
                }
            }
            if (overlap) break;
        }
        if (!overlap) {
            m_fields->m_hasVerified = true;
            if (GameManager::get()->getGameVariable("0128")) PlatformToolbox::hideCursor();
            m_editorUI->onPlaytest(nullptr);
            return;
        }

        auto po = createQuickPopup("Warning", "Your jump keybind seems to be set to the same as your up keybind. If you don't change it, you will not be able to input up.", "Ok", "Keybind Config", [this](auto, bool btn2){
            if (btn2) {
                // :/
                (this->*(menu_selector(MoreOptionsLayer::onKeybindings)))(nullptr);
            } else {
                m_fields->m_hasVerified = true;
                if (GameManager::get()->getGameVariable("0128")) PlatformToolbox::hideCursor();
                m_editorUI->onPlaytest(nullptr);
            }
        }, false);
        po->m_noElasticity = !bounce;
        po->show();
    }
#endif
    void onPlaytest() {
        if (!m_objects) return LevelEditorLayer::onPlaytest();
        runChecks(m_objects);
        LevelEditorLayer::onPlaytest();
        if (auto jsLayer = static_cast<JSUILayer*>(m_uiLayer)) {
            jsLayer->fixVisibility();
            if (auto node = jsLayer->m_fields->m_joystickNode) {
                node->m_currentInput = CCPoint{0, 0};
            }
        }
        updateVal(this, 3740, 1);
#ifdef GEODE_IS_DESKTOP
        if (enableJoystick && !m_fields->m_hasVerified) {
            m_editorUI->onPlaytest(nullptr);
            PlatformToolbox::showCursor();
            popup();
        }
#endif
    }
};

class $modify(JSPL, PlayLayer) {
    struct Fields {
        bool m_hasVerified = false;
    };
#ifdef GEODE_IS_DESKTOP
    void popup(bool bounce = true) {
        auto jbinds = keybinds::BindManager::get()->getBindsFor("robtop.geometry-dash/jump-p1");
        auto mbinds = keybinds::BindManager::get()->getBindsFor("joystick_up"_spr);
        bool overlap = false;
        for (const auto& jbind : jbinds) {
            for (const auto& mbind : mbinds) {
                if (jbind->isEqual(mbind)) {
                    overlap = true;
                    break;
                }
            }
            if (overlap) break;
        }
        if (!overlap) {
            m_fields->m_hasVerified = true;
            if (GameManager::get()->getGameVariable("0128")) PlatformToolbox::hideCursor();

            return;
        }

        queueInMainThread([bounce, this] {
            auto po = createQuickPopup("Warning", "Your jump keybind seems to be set to the same as your up keybind. If you don't change it, you will not be able to input up.", "Ok", "Keybind Config", [this](auto, bool btn2){
                if (btn2) {
                    // :/
                    (this->*menu_selector(MoreOptionsLayer::onKeybindings))(nullptr);
                } else {
                    m_fields->m_hasVerified = true;
                    if (GameManager::get()->getGameVariable("0128")) PlatformToolbox::hideCursor();
                }
            }, false);
                po->m_scene = getParent();
                po->m_noElasticity = !bounce;
                po->show();
            });
    }
#endif
    void setupHasCompleted() {
        if (!m_objects) return PlayLayer::setupHasCompleted();
        runChecks(m_objects);
        PlayLayer::setupHasCompleted();
        updateVal(this, 3740, 1);
        if (auto jsLayer = static_cast<JSUILayer*>(m_uiLayer)) {
            jsLayer->fixVisibility();
        }
#ifdef GEODE_IS_DESKTOP
        if (enableJoystick && !m_fields->m_hasVerified) {
            queueInMainThread([this] {
                pauseGame(false);
            });
            PlatformToolbox::showCursor();
            popup(false);
        }
#endif

    }

    void resetLevel() {
        PlayLayer::resetLevel();
        updateVal(this, 3740, 1);
        if (auto jsLayer = static_cast<JSUILayer*>(m_uiLayer)) {
            jsLayer->fixVisibility();
        }
    }
    #if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_ARM_MAC)
    void resume() {
        PlayLayer::resume();
        if (auto jsLayer = static_cast<JSUILayer*>(m_uiLayer)) {
            jsLayer->fixVisibility();
        }
    }
    #endif
};

class $modify(LTLSL, LevelSettingsLayer) {
    
    struct Fields {
        TextGameObject* m_obj = nullptr;
        CCMenuItemToggler* m_toggle;
    };

    bool init(LevelSettingsObject* settings, LevelEditorLayer* editor) {
        if (!LevelSettingsLayer::init(settings, editor)) return false;
        if (!editor || !editor->m_objects) return true;

        for (auto obj : CCArrayExt<GameObject*>(editor->m_objects)) {
            if (obj->m_objectID == 914) {
                if (auto txt = static_cast<TextGameObject*>(obj)) {
                    if (txt->m_text == "--enable-joystick") {
                        m_fields->m_obj = txt;
                    }
                }
            } 
        }

        CCMenuItemToggler* toggler = CCMenuItemExt::createTogglerWithStandardSprites(.7f, [this, editor](auto){
            if (m_fields->m_obj) {
                editor->m_editorUI->deselectObject(m_fields->m_obj);
                editor->m_editorUI->deleteObject(m_fields->m_obj, false);
                m_fields->m_obj = nullptr;
            } else {
                TextGameObject* obj = static_cast<TextGameObject*>(editor->m_editorUI->cre
// ... (Rest of the JSUILayer, JSLEL, JSPL, LTLSL, and JSPO classes remain largely the same, 
// but ensure they call the updated handleInput and updateVal signatures)
