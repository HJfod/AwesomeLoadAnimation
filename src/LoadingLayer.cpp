#include <Geode.hpp>
#include <array>
#include <thread>
#include <mutex>
#include <array>
#include <random>

USE_GEODE_NAMESPACE();

bool g_differentAddImage = false;
bool g_bypassFrameCache = false;
LoadingLayer* g_loadingReady = nullptr;
bool g_useMulti = true;

static std::vector<std::string> g_toMulti;
static Timer perf;

ccColor3B rainbow(float l) {
    float r, g, b;
         if (l<380.f) r=      0.00f;
    else if (l<400.f) r=0.05f-0.05f*sin(M_PI*(l-366.f)/ 33.f);
    else if (l<435.f) r=      0.31f*sin(M_PI*(l-395.f)/ 81.f);
    else if (l<460.f) r=      0.31f*sin(M_PI*(l-412.f)/ 48.f);
    else if (l<540.f) r=      0.00f;
    else if (l<590.f) r=      0.99f*sin(M_PI*(l-540.f)/104.f);
    else if (l<670.f) r=      1.00f*sin(M_PI*(l-507.f)/182.f);
    else if (l<730.f) r=0.32f-0.32f*sin(M_PI*(l-670.f)/128.f);
    else              r=      0.00f;
         if (l<454.f) g=      0.00f;
    else if (l<617.f) g=      0.78f*sin(M_PI*(l-454.f)/163.f);
    else              g=      0.00f;
         if (l<380.f) b=      0.00f;
    else if (l<400.f) b=0.14f-0.14f*sin(M_PI*(l-364.f)/ 35.f);
    else if (l<445.f) b=      0.96f*sin(M_PI*(l-395.f)/104.f);
    else if (l<510.f) b=      0.96f*sin(M_PI*(l-377.f)/133.f);
    else              b=      0.00f;
    return ccColor3B {
        static_cast<GLubyte>(255 * r),
        static_cast<GLubyte>(255 * g),
        static_cast<GLubyte>(255 * b)
    };
}

class TextureWait : public CCObject {
protected:
    std::string m_name;

    bool init(std::string const& pngName) {
        m_name = pngName.substr(0, pngName.size() - 4);
        return true;
    }

public:
    static TextureWait* create(std::string const& pngName) {
        auto ret = new TextureWait;
        if (ret && ret->init(pngName)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    void onMicrowaveDing(CCObject*) {
        auto name = m_name;
        vector_utils::erase(g_toMulti, name);
        CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile((name + ".plist").c_str());
        if (g_loadingReady) {
            g_loadingReady->loadingFinished();
        }
    }
};

class $modify(CustomFrameCache, CCSpriteFrameCache) {
    void addSpriteFramesWithDictionary(CCDictionary* dictionary, CCTexture2D* pobTexture) {
        CCDictionary* metadataDict = (CCDictionary*)dictionary->objectForKey("metadata");
        CCDictionary* framesDict = (CCDictionary*)dictionary->objectForKey("frames");
        int format = 0;
        if (metadataDict != nullptr)  {
            format = metadataDict->valueForKey("format")->intValue();
        }

        CCDictElement* pElement = nullptr;
        CCDICT_FOREACH(framesDict, pElement) {
            CCDictionary* frameDict = (CCDictionary*)pElement->getObject();
            std::string spriteFrameName = pElement->getStrKey();
            CCSpriteFrame* spriteFrame = (CCSpriteFrame*)m_pSpriteFrames->objectForKey(spriteFrameName);
            if (spriteFrame) {
                continue;
            }
            
            if (format == 0) {
                float x = frameDict->valueForKey("x")->floatValue();
                float y = frameDict->valueForKey("y")->floatValue();
                float w = frameDict->valueForKey("width")->floatValue();
                float h = frameDict->valueForKey("height")->floatValue();
                float ox = frameDict->valueForKey("offsetX")->floatValue();
                float oy = frameDict->valueForKey("offsetY")->floatValue();
                int ow = frameDict->valueForKey("originalWidth")->intValue();
                int oh = frameDict->valueForKey("originalHeight")->intValue();
                ow = abs(ow);
                oh = abs(oh);
                spriteFrame = new CCSpriteFrame();
                spriteFrame->initWithTexture(
                    pobTexture, 
                    CCRectMake(x, y, w, h), 
                    false,
                    CCPointMake(ox, oy),
                    CCSizeMake((float)ow, (float)oh)
                );
            } else if (format == 1 || format == 2) {
                CCRect frame = CCRectFromString(frameDict->valueForKey("frame")->getCString());
                bool rotated = false;

                if (format == 2) {
                    rotated = frameDict->valueForKey("rotated")->boolValue();
                }

                CCPoint offset = CCPointFromString(frameDict->valueForKey("offset")->getCString());
                CCSize sourceSize = CCSizeFromString(frameDict->valueForKey("sourceSize")->getCString());

                spriteFrame = new CCSpriteFrame();
                spriteFrame->initWithTexture(pobTexture, 
                    frame,
                    rotated,
                    offset,
                    sourceSize
                );
            } else if (format == 3) {
                CCSize spriteSize = CCSizeFromString(frameDict->valueForKey("spriteSize")->getCString());
                CCPoint spriteOffset = CCPointFromString(frameDict->valueForKey("spriteOffset")->getCString());
                CCSize spriteSourceSize = CCSizeFromString(frameDict->valueForKey("spriteSourceSize")->getCString());
                CCRect textureRect = CCRectFromString(frameDict->valueForKey("textureRect")->getCString());
                bool textureRotated = frameDict->valueForKey("textureRotated")->boolValue();

                CCArray* aliases = (CCArray*) (frameDict->objectForKey("aliases"));

                // the OG cocos2d code has a loop here for 
                // aliases. rob seems to have removed that tho
                // and just keeps this now unnecessary
                // allocation
                CCString * frameKey = new CCString(spriteFrameName);
                frameKey->release();

                spriteFrame = new CCSpriteFrame();
                spriteFrame->initWithTexture(
                    pobTexture,
                    CCRectMake(textureRect.origin.x, textureRect.origin.y, spriteSize.width, spriteSize.height),
                    textureRotated,
                    spriteOffset,
                    spriteSourceSize
                );
            }
            m_pSpriteFrames->setObject(spriteFrame, spriteFrameName);
            spriteFrame->release();
        }
    }

    void addSpriteFramesWithFile(const char* pszPlist) {
        if (g_differentAddImage && vector_utils::contains<std::string>(
            g_toMulti, [pszPlist](std::string const& s) -> bool {
                return std::string(pszPlist).substr(0, strlen(pszPlist) - 6) == s;
            }
        )) { return; }

        auto notLoaded = m_pLoadedFileNames->find(pszPlist) == m_pLoadedFileNames->end();

        if (notLoaded) {
            auto dict = CCContentManager::sharedManager()->addDict(pszPlist, false);

            std::string texturePath("");

            CCDictionary* metadataDict = (CCDictionary*)dict->objectForKey("metadata");
            if (metadataDict) {
                texturePath = metadataDict->valueForKey("textureFileName")->getCString();
            }

            if (texturePath.empty())  {
                texturePath = pszPlist;
                size_t startPos = texturePath.find_last_of("."); 
                texturePath = texturePath.erase(startPos);
                texturePath = texturePath.append(".png");
            }

            CCTexture2D* pTexture = CCTextureCache::sharedTextureCache()->addImage(texturePath.c_str(), false);
            if (pTexture) {
                CustomFrameCache::addSpriteFramesWithDictionary(dict, pTexture);
                m_pLoadedFileNames->insert(pszPlist);
            }
        }
    }
};

class $modify(CCTextureCache) {
    CCTexture2D* addImage(const char* image, bool idk) {
        if (!g_differentAddImage || !vector_utils::contains<std::string>(
            g_toMulti, [image](std::string const& s) -> bool {
                return std::string(image).substr(0, strlen(image) - 4) == s;
            }
        )) {
            return CCTextureCache::addImage(image, idk);
        }
        this->addImageAsync(
            image,
            TextureWait::create(image),
            callfuncO_selector(TextureWait::onMicrowaveDing)
        );
        return nullptr;
    }
};

class $modify(CustomLoadingLayer, LoadingLayer) {
    bool init(bool fromReload) {
        if (!LoadingLayer::init(fromReload))
            return false;

        perf.reset();

        if (g_useMulti) {
            g_toMulti = {
                "GJ_GameSheet",
                "GJ_GameSheet02",
                "GJ_GameSheet03",
                "GJ_GameSheet04",
                "GJ_GameSheetGlow",
                "FireSheet_01",
                "GJ_ShopSheet",
                "CCControlColourPickerSpriteSheet",
            };
            g_differentAddImage = true;
            g_bypassFrameCache = true;
            g_loadingReady = nullptr;
        }

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        this->m_sliderBar->retain();
        this->removeAllChildren();

        constexpr const int displayCount = 4;

        this->setPosition(winSize.width, 0);

        auto menu = CCMenu::create();
        std::set<int> used;
        for (int i = 0; i < displayCount; i++) {
            std::random_device r;
            std::default_random_engine el(r());
            std::uniform_int_distribution<int> dist(0, 6);

            int ix = -1;
            while (ix == -1 || used.count(ix)) {
                ix = dist(el);
            }
            used.insert(ix);
            auto icon = CCSprite::create(
                CCString::createWithFormat("icon%02d.png"_spr, ix)->getCString()
            );
            if (icon) {
                auto color = cocos::lighten3B(
                    rainbow(i / static_cast<float>(displayCount) * 320.f + 430.f),
                    80
                );
                icon->setScale(.35f);
                icon->setOpacity(0);
                icon->runAction(CCSequence::create(
                    CCMoveBy::create(.0f, { .0f, -40.f }),
                    CCDelayTime::create(i * .1f),
                    CCSpawn::create(
                        CCFadeTo::create(.2f, 255),
                        CCEaseOut::create(
                            CCSpawn::create(
                                CCMoveBy::create(.2f, { .0f, 50.f }),
                                CCScaleTo::create(.2f, .3f, .4f),
                                CCTintTo::create(.2f, color.r, color.g, color.b),
                                nullptr
                            ), 2.f
                        ),
                        nullptr
                    ),
                    CCRepeat::create(
                        CCSequence::create(
                            CCSpawn::create(
                                CCEaseBackOut::create(
                                    CCSpawn::create(
                                        CCScaleTo::create(.2f, .35f, .35f),
                                        CCMoveBy::create(.2f, { .0f, -10.f }),
                                        nullptr
                                    )
                                ),
                                CCTintTo::create(.2f, 255, 255, 255),
                                nullptr
                            ),
                            CCDelayTime::create(.4f),
                            CCEaseOut::create(
                                CCSpawn::create(
                                    CCMoveBy::create(.2f, { .0f, 10.f }),
                                    CCScaleTo::create(.2f, .3f, .4f),
                                    CCTintTo::create(.2f, color.r, color.g, color.b),
                                    nullptr
                                ), 2.f
                            ),
                            nullptr
                        ),
                        324853
                    ),
                    nullptr
                ));
                menu->addChild(icon);
            }
        }
        menu->alignItemsHorizontallyWithPadding(7.f);

        auto label = CCLabelBMFont::create("Loading Geode...", "chatFont.fnt");
        label->setPosition(.0f, -40.f);
        label->setScale(.65f);
        label->setOpacity(0);
        label->runAction(CCFadeTo::create(.3f, 255));
        menu->addChild(label);
        
        menu->setPosition(-winSize.width / 2, winSize.height / 2);
        this->addChild(menu);

        return true;
    }

    void gotoMenu() {
        CCDirector::sharedDirector()->replaceScene(
            CCTransitionFade::create(
                1.f, MenuLayer::scene(m_fromRefresh)
            )
        );
    }

    void loadingFinished() {
        if (g_useMulti) {
            g_differentAddImage = false;
            g_loadingReady = this;
            if (g_toMulti.empty()) {
                CustomLoadingLayer::gotoMenu();
            }
        } else {
            LoadingLayer::loadingFinished();
        }
    }
};

