#pragma once
#ifndef ANGKASA1_SANDBOX_TEXT_TEXT_CONTROLLER_H
#define ANGKASA1_SANDBOX_TEXT_TEXT_CONTROLLER_H

#include "FontController.h"

namespace sandbox
{

  // This ought to really be a system in the engine.
  class TextController
  {
  private:

    struct TextEntry
    {
      astl::String Str; // This should be a string view in the future.
      astl::Ref<Font> pFont;
      math::vec2 Pos;
      math::vec3 Color;
      float32 FontSizePx; // In pixels!
    };

    struct GlyphQuad
    {
      VertexInformation Vertices;
      IndexInformation Indices;
      Handle<SMemoryBuffer> Hnd;
    };

    struct GlyphInstanceAttrib
    {
      math::mat4 Transform;
      math::vec2 Uv[4];
      math::vec3 Color;
    };

    GlyphQuad Quad;
    FontController FontCtrl;
    Handle<SMemoryBuffer> GlyphIboHnd;
    astl::Array<TextEntry> Texts;
    astl::Array<GlyphInstanceAttrib> InstanceAttrib;
    astl::Ref<Font> DefaultFont;
    astl::Ref<IRenderSystem> pRenderer;
    size_t MaxNumGlyphs;

    void AllocateGlyphBuffer(size_t NumGlyphs);

  public:

    TextController(IRenderSystem& InRenderer, EngineImpl& InEngine);
    ~TextController();

    DELETE_COPY_AND_MOVE(TextController)

    void Initialize(size_t InitialMaxGlyphCount);
    void Terminate();

    void SetDefaultFont(Handle<Font> Hnd);
    Handle<Font> LoadFont(const astl::FilePath& Path, uint32 Size);
    Handle<SImage> GetFontAtlasHandle(Handle<Font> Hnd);
    Handle<SMemoryBuffer> GetGlyphInstanceBufferHandle() const;
    void Print(astl::String&& Text, const math::vec2 Pos, float32 SizePx = 16.f, math::vec3 Color = { 1.f, 1.f, 1.f }, Handle<Font> FontHnd = INVALID_HANDLE);
    void Finalize(math::mat4& ProjConstant, uint32& NumInstances, float32 CanvasWidth, float32 CanvasHeight);

    VertexInformation& GetGlyphQuadVertexInformation();
    IndexInformation& GetGlyphQuadIndexInformation();
  };

}

#endif // !ANGKASA1_SANDBOX_TEXT_TEXT_CONTROLLER_H
