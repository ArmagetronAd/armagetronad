#include "doctest.h"
#include "gCycleVisuals.h"
#include "rScreen.h"
#include "MockConsole.h"

#ifndef DEDICATED
namespace
{
struct BikeResources
{
    rModel *body = nullptr, *front = nullptr, *rear = nullptr, *custom = nullptr;
    gTextureCycle *bodyTexture = nullptr, *wheelTexture = nullptr, *customTexture = nullptr;
    bool moviepack = false;
    ~BikeResources()
    {
        delete bodyTexture;
        delete wheelTexture;
        delete customTexture;
    }
    void Commit(gCycleVisuals& loaded)
    {
        loaded.Commit(body, front, rear, custom, bodyTexture, wheelTexture, customTexture, moviepack);
    }
};

struct ResourceFixture
{
    MockConsole quiet;
    bool gl = sr_glOut;
    int textureMode = rTextureGroups::TextureMode[rTextureGroups::TEX_OBJ];
    ResourceFixture()
    {
        sr_glOut = false;
        rTextureGroups::TextureMode[rTextureGroups::TEX_OBJ] = GL_LINEAR;
    }
    ~ResourceFixture()
    {
        rTextureGroups::TextureMode[rTextureGroups::TEX_OBJ] = textureMode;
        sr_glOut = gl;
    }
};
} // namespace

TEST_CASE("Replacing a moviepack presentation commits its model type and textures together")
{
    ResourceFixture fixture;
    BikeResources bike;
    rColor colour(1, .4f, .02f);
    {
        gCycleVisuals first(colour);
        REQUIRE(first.LoadModel(false));
        bike.Commit(first);
    }
    REQUIRE(bike.bodyTexture);
    // Represent the previous single-model moviepack view using borrowed model
    // data and a real owned texture. No OpenGL context or game simulation runs.
    bike.custom = bike.body;
    bike.body = nullptr;
    bike.customTexture = bike.bodyTexture;
    bike.bodyTexture = nullptr;
    bike.moviepack = true;
    {
        gCycleVisuals replacement(colour);
        REQUIRE(replacement.LoadModel(false));
        auto* nextTexture = replacement.bodyTexture;
        auto* oldTexture = bike.customTexture;
        bike.Commit(replacement);
        CHECK_FALSE(bike.moviepack);
        CHECK(bike.body != nullptr);
        CHECK(bike.front != nullptr);
        CHECK(bike.rear != nullptr);
        CHECK(bike.bodyTexture == nextTexture);
        CHECK(bike.customTexture == nullptr);
        CHECK(replacement.customTexture == oldTexture);
    }
    CHECK(bike.bodyTexture != nullptr);
}

TEST_CASE("Repeated resource replacement retains the requested preference and the loaded format")
{
    ResourceFixture fixture;
    BikeResources bike;
    for (bool requested : {false, true, false, true, false})
    {
        gCycleVisuals replacement(rColor(.1f, .85f, 1));
        REQUIRE(replacement.LoadModel(requested));
        bool actualFormat = replacement.mpType;
        CHECK(replacement.mpPreference == int(requested));
        bike.Commit(replacement);
        CHECK(bike.moviepack == actualFormat);
        if (actualFormat)
        {
            CHECK(bike.custom != nullptr);
            CHECK(bike.customTexture != nullptr);
        }
        else
        {
            CHECK(bike.body != nullptr);
            CHECK(bike.bodyTexture != nullptr);
            CHECK(bike.wheelTexture != nullptr);
        }
    }
}
#endif
