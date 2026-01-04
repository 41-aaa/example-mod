#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class JoystickNode : public CCMenu {
    protected:
        bool init() override;

        bool ccTouchBegan(CCTouch *touch, CCEvent *event) override;
        void ccTouchEnded(CCTouch *touch, CCEvent *event) override;
        void ccTouchMoved(CCTouch *touch, CCEvent *event) override;
        void ccTouchCancelled(CCTouch *touch, CCEvent *event) override;

        void draw() override;
        void registerWithTouchDispatcher() override;

        CCSprite *m_bg = nullptr;
        CCSprite *m_center = nullptr;
    public:
        static JoystickNode *create();
        void handleInput(GJBaseGameLayer *layer, CCPoint input, CCPoint old);
        void fakePosition();
        
        bool m_twoPlayer = false;
        CCPoint m_currentInput = {0, 0};
};