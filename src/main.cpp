#include <Geode/Geode.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/StartPosObject.hpp>
#include <Geode/modify/UILayer.hpp>

using namespace geode::prelude;

std::vector<StartPosObject*> startPos = {};
int selectedStartpos = 0;

bool a = false;

CCLabelBMFont* label;
CCMenu* menu;

$execute
{
    #ifdef GEODE_IS_ANDROID64
    //1F2003D5
    Mod::get()->patch(reinterpret_cast<void *>(geode::base::get() + 0x82803c), {0x1F, 0x20, 0x03, 0xD5});
    #endif
}

void switchToStartpos(int incBy, bool actuallySwitch = true)
{
    selectedStartpos += incBy;

    if (selectedStartpos < -1)
        selectedStartpos = startPos.size() - 1;
        
    if (selectedStartpos >= startPos.size())
        selectedStartpos = -1;

    log::info("startpos: {}", selectedStartpos);

    if (actuallySwitch)
    {
        StartPosObject* startPosObject = selectedStartpos == -1 ? nullptr : startPos[selectedStartpos];

        // delete the startposcheckpoint (see playlayer_resetlevel line 148 in ida)

        #ifdef GEODE_IS_WINDOWS
        int offset = 0xB85;// 0xA6A;
        #endif
        
        #ifdef GEODE_IS_ANDROID32
        int offset = 0xB93 - 0x16;
        #endif

        #ifdef GEODE_IS_ANDROID64
        int offset = 0x38c0 / 4;
        #endif

        #ifndef GEODE_IS_ANDROID64

        int* startPosCheckpoint = (int*)GameManager::get()->getPlayLayer() + offset;//2949
        *startPosCheckpoint = 0;
        
        #endif

        if (!startPosObject && selectedStartpos != -1)
            return;

        //reinterpret_cast<void(__thiscall*)(PlayLayer*, StartPosObject*)>(base::get() + 0x199E90)(GameManager::get()->getPlayLayer(), startPosObject);
        PlayLayer::get()->setStartPosObject(startPosObject);

        GameManager::get()->getPlayLayer()->resetLevel();

        // apparently you have to start music manually since gd only does it if you dont have a startpos???? (see
        // playlayer_resetlevel line 272 in ida)
        GameManager::get()->getPlayLayer()->startMusic();
    }

    std::stringstream ss;
    ss << selectedStartpos + 1;
    ss << "/";
    ss << startPos.size();

    label->setString(ss.str().c_str());
}

class StartposSwitcher
{
    public:
        void onLeft(CCObject*)
        {
            switchToStartpos(-1);
        }

        void onRight(CCObject*)
        {
            switchToStartpos(1);
        }
};

void onDown(enumKeyCodes key)
{
    if (startPos.size() == 0)
        return;

    std::vector<enumKeyCodes> leftKeys = {
        enumKeyCodes::KEY_Subtract,
        enumKeyCodes::KEY_OEMComma,
        enumKeyCodes::CONTROLLER_Left,
    };

    std::vector<enumKeyCodes> rightKeys = {
        enumKeyCodes::KEY_Add,
        enumKeyCodes::KEY_OEMPeriod,
        enumKeyCodes::CONTROLLER_Right,
    };

    #ifndef GEODE_IS_ANDROID

    if (!PlayLayer::get()->m_player1->m_isPlatformer)
    {
        //leftKeys.push_back(enumKeyCodes::KEY_W);
        leftKeys.push_back(enumKeyCodes::KEY_Left);

        //rightKeys.push_back(enumKeyCodes::KEY_D);
        rightKeys.push_back(enumKeyCodes::KEY_Right);
    }

    if (!PlayLayer::get()->m_isPracticeMode)
    {
        leftKeys.push_back(enumKeyCodes::KEY_Z);

        rightKeys.push_back(enumKeyCodes::KEY_X);
    }

    #endif

    bool left = false;
    bool right = false;

    for (size_t i = 0; i < leftKeys.size(); i++)
    {
        if (key == leftKeys[i])
            left = true;
    }

    for (size_t i = 0; i < rightKeys.size(); i++)
    {
        if (key == rightKeys[i])
            right = true;
    }
    

    if (left)
    {
        selectedStartpos--;

        if (selectedStartpos < -1)
            selectedStartpos = startPos.size() - 1;
    }

    if (right)
    {
        selectedStartpos++;
        
        if (selectedStartpos >= startPos.size())
            selectedStartpos = -1;
    }

    if (left || right)
    {
        switchToStartpos(0);
    }
}

class $modify (CCKeyboardDispatcher)
{
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool idk)
    {
        if (!CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, idk))     
            return false;

        if (PlayLayer::get() && down)
        {
            onDown(key);
        }

        return true;
    }
};

class $modify(PlayLayer)
{
    static PlayLayer* create(GJGameLevel* p0, bool p1, bool p2)
    {
        startPos.clear();
        selectedStartpos = -1;

        auto res = PlayLayer::create(p0, p1, p2);

        switchToStartpos(0, false);

        if (startPos.size() == 0)
            menu->setVisible(false);

        return res;
    }
};

class $modify (UILayer)
{
    bool init(GJBaseGameLayer* idk)
    {
        if (!UILayer::init(idk))
            return false;

        if (!typeinfo_cast<PlayLayer*>(idk))
            return true;

        menu = CCMenu::create();
        menu->setPosition(ccp(CCDirector::get()->getWinSize().width / 2, 25));
        menu->setContentSize(ccp(0, 0));
        menu->setScale(0.6f);
        menu->setVisible(Mod::get()->getSettingValue<bool>("show-ui"));

        label = CCLabelBMFont::create("0/0", "bigFont.fnt");
        label->setPosition(ccp(0, 0));
        label->setOpacity(100);

        auto leftSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
        leftSpr->setOpacity(100);

        auto leftBtn = CCMenuItemSpriteExtra::create(leftSpr, menu, menu_selector(StartposSwitcher::onLeft));
        leftBtn->setContentSize(leftBtn->getContentSize() * 5);
        leftSpr->setPosition(leftBtn->getContentSize() / 2);
        leftBtn->setPosition(ccp(-85, 0));

        auto rightSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
        rightSpr->setFlipX(true);
        rightSpr->setOpacity(100);

        auto rightBtn = CCMenuItemSpriteExtra::create(rightSpr, menu, menu_selector(StartposSwitcher::onRight));
        rightBtn->setContentSize(rightBtn->getContentSize() * 5);
        rightSpr->setPosition(rightBtn->getContentSize() / 2);
        rightBtn->setPosition(ccp(85, 0));

        if (PlatformToolbox::isControllerConnected()) // this shits not working :(
        {
            auto leftCtrl = CCSprite::createWithSpriteFrameName("controllerBtn_DPad_Left_001.png");
            leftCtrl->setPosition(ccp(0, -15));
            leftSpr->addChild(leftCtrl);

            auto rightCtrl = CCSprite::createWithSpriteFrameName("controllerBtn_DPad_Right_001.png");
            rightCtrl->setPosition(ccp(0, -15));
            rightSpr->addChild(rightCtrl);
        }

        menu->addChild(label);
        menu->addChild(leftBtn);
        menu->addChild(rightBtn);
        this->addChild(menu);
        return true;
    }
};

class $modify (StartPosObject)
{
    virtual bool init()
    {
        if (!StartPosObject::init())
            return false;

        //log::info("startpos");

        startPos.push_back(static_cast<StartPosObject*>(this));
        selectedStartpos = -1;

        return true;
    }
};