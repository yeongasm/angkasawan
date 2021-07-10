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

  enum EGlyphTexCoord : uint32
  {
    Glyph_TexCoord_TL = 0,
    Glyph_TexCoord_TR = 1,
    Glyph_TexCoord_BL = 2,
    Glyph_TexCoord_BR = 3,
    Glyph_TexCoord_Max = 4,
  };

  struct Font;

  struct Glyph
  {
    math::vec2 Pos;
    math::vec2 Dim;
    math::vec2 Bearing;
    math::vec2  TexCoords[Glyph_TexCoord_Max];
    float32 Advance;
  };

  struct Font
  {
    struct AtlasInfo
    {
      Handle<SImage> Hnd;
      uint32 Width;
      uint32 Height;
    };
    astl::Array<Glyph> Characters;
    FT_Face Face;
    uint32 SizePx;
    float32 FSizePx;
    float32 FMaxFontHeight;
    AtlasInfo Atlas;
    EFontEncoding Encoding;
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

    Handle<Font> LoadFont(const astl::FilePath& Path, EFontEncoding Encoding, uint32 FontSizeInPixels = 16, uint32 Width = 4096, uint32 Height = 4096, uint32 HorzDpi = 96, uint32 VertDpi = 96);
    const astl::Ref<Font> GetFont(Handle<Font> Hnd);
    void UnloadFont(Handle<Font>& Hnd);

    const uint32 GetNumOfFonts() const;
  };

}

#endif // !ANGKASA1_SANDBOX_TEXT_FONT_CONTROLLER_H
