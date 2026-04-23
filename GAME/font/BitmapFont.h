#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "Data/Render/CPUData/MaterialConfig.h"
#include <unordered_map>
#include "Vector4.h"
#include <memory>

class kEngine;
class SimpleSprite;
struct Vector2;
struct Vector4;

/// <summary>
/// ビットマップフォント描画システム
/// テクスチャに並べた文字をスプライトで描画する
/// </summary>
class BitmapFont {
public:
	/// テキスト揃え
	enum class Align {
		Left,    /// 左揃え（positionが文字列の左上）
		Center,  /// 中央揃え（positionが文字列の上辺中央）
		Right,   /// 右揃え（positionが文字列の右上）
	};

	BitmapFont() = default;
	~BitmapFont();

	/// <summary>
	/// 初期化（JSONからフォントマップを読み込み）
	/// </summary>
	void Initialize(kEngine* system,
		const std::string& fontMapPath = "GAME/resources/font/font_map.json");

	// publicに追加
	int GetGlyphMapSize() const { return (int)glyphMap_.size(); }

	/// <summary>
	/// テキスト描画
	/// </summary>
	/// <param name="text">表示する文字列</param>
	/// <param name="position">表示位置</param>
	/// <param name="heightPx">文字の高さ（ピクセル）</param>
	/// <param name="align">テキスト揃え</param>
	/// <param name="layer">描画レイヤー（Z値）</param>
	/// <param name="color">文字色（未実装、将来用）</param>
	void RenderText(const std::string& text,
		Vector2 position, float heightPx,
		Align align = Align::Left,
		float layer = 5.0f,
		Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f });

	/// <summary>
	/// 解放
	/// </summary>
	void Cleanup();

private:

	// フォント用に作る shared MaterialConfig キャッシュ
	// key は "texIdx:colorx:colory:colorz:colorw"
	std::unordered_map<std::string, std::shared_ptr<MaterialConfig>> fontMaterialCache_;

	// キャッシュ取得ヘルパー
	std::shared_ptr<MaterialConfig> GetFontMaterial(int textureIndex, Vector4 color);
	kEngine* system_ = nullptr;

	/// 1文字分のフォント情報
	struct GlyphInfo {
		int textureIndex = 0;  /// どのテクスチャか（0:kana, 1:ascii, 2:kanji）
		int row = 0;           /// テクスチャ内の行
		int col = 0;           /// テクスチャ内の列
		bool isHalfWidth = false; /// 半角かどうか
	};

	/// テクスチャ情報
	struct FontTexture {
		int handle = 0;
		int cellWidth = 0;
		int cellHeight = 0;
	};

	/// テクスチャハンドル（0:kana, 1:ascii, 2:kanji）
	FontTexture textures_[3] = {};
	int textureCount_ = 0;

	/// 文字→グリフ情報のマップ
	std::unordered_map<std::string, GlyphInfo> glyphMap_;

	/// フォールバック文字のグリフ情報
	GlyphInfo fallbackGlyph_{};
	bool hasFallback_ = false;

	/// スプライトプール（描画用に再利用）
	std::vector<SimpleSprite*> spritePool_;
	int spritePoolUsed_ = 0;

	/// 1行あたりの列数
	int colsPerRow_ = 16;

	/// <summary>
	/// テクスチャの行配列からグリフマップを構築
	/// </summary>
	void BuildGlyphMap(const std::vector<std::string>& rows,
		int textureIndex, bool isHalfWidth);

	/// <summary>
	/// UTF-8文字列を1文字ずつ分解する
	/// </summary>
	static std::vector<std::string> SplitUTF8(const std::string& text);

	/// <summary>
	/// 文字列の合計幅を計算する（ピクセル）
	/// </summary>
	float CalcTextWidth(const std::vector<std::string>& chars, float heightPx);

	/// <summary>
	/// スプライトプールからスプライトを取得（足りなければ追加生成）
	/// </summary>
	SimpleSprite* GetPooledSprite();

public:
	void BeginFrame();
};
