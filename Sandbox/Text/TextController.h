#pragma once
#ifndef ANGKASA1_SANDBOX_TEXT_TEXT_CONTROLLER_H
#define ANGKASA1_SANDBOX_TEXT_TEXT_CONTROLLER_H

#include "FontController.h"

namespace sandbox
{

  struct GlyphConstant
  {
    math::mat4 Projection;
    math::vec2 Uv[4];
    math::vec3 Color;
  };

  // This ought to really be a system in the engine.
  class TextController
  {
  private:

    struct TextEntry
    {
      astl::String Str;
      astl::Ref<Font> pFont;
      uint32 FontSize; // In pixels!
      math::vec2 Pos;
      math::vec3 Color;
    };

    struct GlyphQuad
    {
      VertexInformation Vertices;
      IndexInformation Indices;
      Handle<SMemoryBuffer> Hnd;
    };

    GlyphQuad Quad;
    FontController FontCtrl;
    astl::Array<TextEntry> Texts;
    astl::Ref<Font> DefaultFont;
    astl::Ref<IRenderSystem> pRenderer;

  public:

    TextController(IRenderSystem& InRenderer, EngineImpl& InEngine);
    ~TextController();

    DELETE_COPY_AND_MOVE(TextController)

    void Initialize();
    void Terminate();

    void SetDefaultFont(Handle<Font> Hnd);
    Handle<Font> LoadFont(const astl::FilePath& Path, uint32 Size);
    Handle<SImage> GetFontAtlasHandle(Handle<Font> Hnd);
    void Print(astl::String&& Text, const math::vec2 Pos, uint32 Size = 16, math::vec3 Color = { 0.f, 0.f, 0.f }, Handle<Font> FontHnd = INVALID_HANDLE);
    void Finalize(astl::Array<math::mat4>& OutTransform, astl::Array<GlyphConstant>& OutConstants);

    VertexInformation& GetGlyphQuadVertexInformation();
    IndexInformation& GetGlyphQuadIndexInformation();
  };

}

#endif // !ANGKASA1_SANDBOX_TEXT_TEXT_CONTROLLER_H
