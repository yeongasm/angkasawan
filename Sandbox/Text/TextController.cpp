#include "TextController.h"

namespace sandbox
{

  TextController::TextController(IRenderSystem& InRenderer, EngineImpl& InEngine) :
    FontCtrl{ InRenderer, InEngine },
    Texts{},
    pRenderer{ &InRenderer }
  {}

  TextController::~TextController() {}

  void TextController::Initialize()
  {
    astl::Ref<IStagingManager> pStaging = &pRenderer->GetStagingManager();
    Quad.Vertices.NumVertices = 4;
    Quad.Vertices.VertexOffset = pStaging->GetVertexBufferOffset() / sizeof(Vertex);

    Quad.Indices.NumIndices = 6;
    Quad.Indices.IndexOffset = pStaging->GetIndexBufferOffset() / sizeof(uint32);

    Vertex quad[4] = {};
    quad[0].Position = { 0.0f, 1.0f, 0.0f };
    quad[1].Position = { 1.0f, 1.0f, 0.0f };
    quad[2].Position = { 0.0f, 0.0f, 0.0f };
    quad[3].Position = { 1.0f, 0.0f, 0.0f };

    uint32 indices[6] = { 0, 1, 2, 1, 3, 2 };

    pStaging->StageVertexData(quad, sizeof(Vertex) * 4);
    pStaging->StageIndexData(indices, sizeof(uint32) * 6);
  }

  void TextController::Terminate()
  {
    FontCtrl.~FontController();
  }

  void TextController::SetDefaultFont(Handle<Font> Hnd)
  {
    astl::Ref<Font> pFont = FontCtrl.GetFont(Hnd);
    VKT_ASSERT(pFont && "Font handle specified is invalid");
    DefaultFont = pFont;
  }

  Handle<Font> TextController::LoadFont(const astl::FilePath& Path, uint32 Size)
  {
    return FontCtrl.LoadFont(Path, EFontEncoding::Font_Encoding_ASCII, Size);
  }

  Handle<SImage> TextController::GetFontAtlasHandle(Handle<Font> Hnd)
  {
    astl::Ref<Font> pFont = FontCtrl.GetFont(Hnd);
    return pFont->Atlas.Hnd;
  }

  void TextController::Print(astl::String&& Text, const math::vec2 Pos, uint32 Size, math::vec3 Color, Handle<Font> FontHnd)
  {
    TextEntry entry;
    entry.pFont = DefaultFont;
    if (FontHnd != INVALID_HANDLE)
    {
      entry.pFont = FontCtrl.GetFont(FontHnd);
      if (!entry.pFont) { entry.pFont = DefaultFont; }
      return;
    }
    entry.FontSize = Size;
    entry.Color = Color;
    entry.Pos = Pos;
    entry.Str = astl::Move(Text);

    Texts.Push(astl::Move(entry));
  }

  void TextController::Finalize(astl::Array<math::mat4>& OutTransform, astl::Array<GlyphConstant>& OutConstants)
  {
    for (TextEntry& text : Texts)
    {
      float32 pos_x = text.Pos.x;
      float32 pos_y = text.Pos.y;

      uint32 uScale = (text.FontSize / text.pFont->SizeInPt);
      float32 fScale = static_cast<float32>(uScale);

      for (astl::String::ConstType c : text.Str)
      {
        const Glyph& character = text.pFont->Characters[c];
        float32 width = character.BR.x - character.TL.x;
        float32 height = (character.BR.y - character.TL.y) * -1;

        pos_x += character.Bearing.x * fScale;
        pos_y -= (height - character.Bearing.y) * fScale;

        math::mat4 transform(1.0f);
        math::Translate(transform, math::vec3(pos_x, pos_y, 0.0f));
        math::Scale(transform, math::vec3(width * fScale, height * fScale, 0.f));
        OutTransform.Push(transform);

        GlyphConstant constant;
        constant.Projection = math::OrthographicRH(0, 800.0f, 0, 600.0f, 0.0f, 100.0f);
        constant.Uv[0] = character.TL;
        constant.Uv[1] = { character.BR.x, character.TL.y };
        constant.Uv[2] = { character.TL.x, character.BR.y };
        constant.Uv[3] = character.BR;
        constant.Color = text.Color;
        OutConstants.Push(astl::Move(constant));

        pos_x += (float32)character.Advance * fScale;
      }
    }
    Texts.Empty();
  }

  VertexInformation& TextController::GetGlyphQuadVertexInformation()
  {
    return Quad.Vertices;
  }

  IndexInformation& TextController::GetGlyphQuadIndexInformation()
  {
    return Quad.Indices;
  }

}
