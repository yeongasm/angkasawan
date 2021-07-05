#pragma once
#ifndef ANGKASA1_SANDBOX_TEXT_FONT_CONTROLLER_H
#define ANGKASA1_SANDBOX_TEXT_FONT_CONTROLLER_H

#include <ft2build.h>
#include <freetype/freetype.h>
#include "Library/Math/Math.h"
#include "Library/Containers/Map.h"
#include "Library/Containers/String.h"
#include "Library/Containers/Path.h"
#include "Library/Containers/Buffer.h"
#include "Renderer.h"
#include "Asset/Assets.h"

namespace sandbox
{
  enum class EFontEncoding : uint32
  {
    Font_Encoding_ASCII = 128
  };

  struct Glyph
  {
    math::vec2 TL;
    math::vec2 BR;
    math::vec2 Bearing;
    uint32 Advance;
  };

  struct Font
  {
    struct AtlasInfo
    {
      Handle<SImage> Hnd;
      uint32 Width;
      uint32 Height;
    };
    astl::Array<Glyph>    Characters;
    FT_Face         Face;
    uint32          SizeInPt;
    uint32          Dpi;
    AtlasInfo       Atlas;
    EFontEncoding   Encoding;
  };

  class FontController
  {
  private:

    FT_Library Ft;
    astl::Map<uint32, Font> Fonts;
    astl::Ref<IRenderSystem> pRenderer;
    astl::Ref<EngineImpl> pEngine;

    void CreateGlyphAtlas(Font& InFont, const astl::Buffer<uint8>& Pixels);
    void UnloadFont(Font& InFont);

  public:

    FontController(IRenderSystem& InRenderer, EngineImpl& InEngine);
    ~FontController();

    DELETE_COPY_AND_MOVE(FontController)

    Handle<Font> LoadFont(const astl::FilePath& Path, EFontEncoding Encoding, uint32 FontSizeInPixels = 16, uint32 Dpi = 96);
    const astl::Ref<Font> GetFont(Handle<Font> Hnd);
    void UnloadFont(Handle<Font>& Hnd);

    uint32 GetFontMaxSizePt(Handle<Font> Hnd);
    uint32 GetFontMaxSizePx(Handle<Font> Hnd);

    uint32 ConvertPixelsToPoints(uint32 Pixels, uint32 Dpi);
    uint32 ConvertPointsToPixels(uint32 Points, uint32 Dpi);
  };

}

#endif // !ANGKASA1_SANDBOX_TEXT_FONT_CONTROLLER_H
