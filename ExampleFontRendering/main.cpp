#include <SDLInstance.h>
#include <Window.h>
#include <../SDLClasses/include/Vector4.h>
#include <iostream>
#include <algorithm>
#include <../ClosestGL/Vector4.h>
#include <../SDLClasses/include/Vector2.h>
#include <ExampleCommon.h>
#include <SDLError.h>
#include <map>
#include <fstream>
#include <sstream>
#include <ClosestGL.h>
#include <type_traits>
#include <string>
#include <Keyboard.h>
using namespace ClosestGL;
using namespace ExampleCommon;

//选定一个颜色类型
using Color = Math::Vector4<float>;

//字体渲染方式
enum class FontMode
{
	Normal,		//仅绘制基本文字
	Outline,	//描边
	Shadow		//软阴影
};


//切割字符串
std::vector<std::wstring> Split(std::wstring str, wchar_t ch)
{
	size_t pos;
	std::vector<std::wstring> vec;
	while ((pos = str.find(ch)) != std::wstring::npos)
	{
		vec.push_back(str.substr(0, pos));
		str = str.substr(pos + 1, str.length() - pos - 1);
	}
	vec.push_back(str);
	return vec;
}

//用于获取字符到UV的映射
//UV的x、y、z、w分别对应该文字在字模上的x坐标、y坐标、宽度和高度
std::map<wchar_t, Math::Vector4<float>> LoadFontIndex(Math::Vector2<size_t> size)
{
	std::map<wchar_t, Math::Vector4<float>> map;

	const std::wstring index[]
	{
		L"C	faceID = 0	x = 0	y = 0	w = 45	h = 64",
		L"G	faceID = 0	x = 45	y = 0	w = 48	h = 64",
		L"L	faceID = 0	x = 93	y = 0	w = 38	h = 64",
		L"e	faceID = 0	x = 131	y = 0	w = 40	h = 64",
		L"l	faceID = 0	x = 171	y = 0	w = 26	h = 64",
		L"o	faceID = 0	x = 197	y = 0	w = 43	h = 64",
		L"s	faceID = 0	x = 0	y = 64	w = 35	h = 64",
		L"t	faceID = 0	x = 35	y = 64	w = 31	h = 64",
		L"体	faceID = 0	x = 66	y = 64	w = 60	h = 64",
		L"向	faceID = 0	x = 126	y = 64	w = 60	h = 64",
		L"场	faceID = 0	x = 186	y = 64	w = 60	h = 64",
		L"字	faceID = 0	x = 0	y = 128	w = 60	h = 64",
		L"染	faceID = 0	x = 60	y = 128	w = 60	h = 64",
		L"渲	faceID = 0	x = 120	y = 128	w = 60	h = 64",
		L"量	faceID = 0	x = 180	y = 128	w = 60	h = 64"
	};

	for (const auto& ch : index)
	{
		wchar_t chName = ch[0];

		const auto ppos = ch.rfind(L"x = ");
		auto position = ch.substr(ppos, ch.length() - ppos);

		position.erase(
			std::remove_if(position.begin(), position.end(), [](auto ch)
			{
				return
					ch == L'x' ||
					ch == L'y' ||
					ch == L'w' ||
					ch == L'h' ||
					ch == L' ' ||
					ch == L'=';
			}),
			position.end()
		);

		auto poses = Split(position, '\t');

		map[chName] = 
		{
			std::stof(poses[0]) / size.x,
			std::stof(poses[1]) / size.y,
			std::stof(poses[2]) / size.x,
			std::stof(poses[3]) / size.y
		};
	}

	return map;
}

//绘制单个文字
void DrawChar(
	Math::Vector4<float> dstRect,	//渲染目标的方框
	Math::Vector4<float> uvRect,	//文字的uv方框
	const Texture::Texture2D<Color>& tex,	//SDF字模纹理
	Texture::Texture2D<Color>& renderTargetTex,	//渲染目标
	FontMode mode,	//渲染模式
	float width)	//宽度
{
	//定义顶点数据类型
	struct Vertex
	{
		Math::Vector4<float> SVPosition;
		Math::Vector2<float> UV;

		static Vertex Lerp(float t,const Vertex& v1, const Vertex& v2)
		{
			return Vertex{
				Math::Lerp(t,v1.SVPosition,v2.SVPosition),
				Math::Lerp(t,v1.UV,v2.UV)
			};
		}
	};

	//计算方框的顶点
	const std::array<Vertex,4> rect =
	{
		Vertex{ { dstRect.x,dstRect.y,1,1 },{ uvRect.x,uvRect.y + uvRect.w } },
		Vertex{ { dstRect.x,dstRect.y + dstRect.w,1,1 },{ uvRect.x,uvRect.y } },
		Vertex{ { dstRect.x + dstRect.z,dstRect.y + dstRect.w,1,1 },{ uvRect.x + uvRect.z,uvRect.y } },
		Vertex{ { dstRect.x + dstRect.z,dstRect.y ,1,1 },{ uvRect.x + uvRect.z,uvRect.y + uvRect.w } },
	};

	//索引列表
	constexpr size_t indicis[] = { 0,1,2,0,3,2 };

	//混合器
	const auto Blender = [](auto src, auto dst) {return ExampleCommon::BlendColorNormal(src, dst); };

	//渲染目标
	RenderPipeline::RenderTarget<1, Color, decltype(Blender)>
		renderTarget{ Blender,{ &renderTargetTex } };

	//如果开启了软阴影，则渲染软阴影
	if (mode == FontMode::Shadow)
	{
		//软阴影的像素着色器
		const auto ShadowPixelShader = [&tex, mode, width](const Vertex& v, const auto&)
		{
			//从字模纹理创建采样器
			Texture::Sampler::Sampler2D<
				std::remove_reference<decltype(tex)>::type,	//纹理类型
				decltype(Texture::Sampler::UVNormalizer::UV2DClamp),	//Clamp纹理坐标限制器
				decltype(Texture::Sampler::Fliters::Bilinear)>	//双线性插值器
				sampler{
				&tex,	//字模纹理
				Texture::Sampler::UVNormalizer::UV2DClamp,	//Clamp纹理坐标限制器
				Texture::Sampler::Fliters::Bilinear };	//双线性插值器

			//获取与字体边缘的距离
			const float distance = sampler.Sample(v.UV).x;

			//阴影的强度
			const float power =
				2 * std::clamp(distance - width,0.0f,1.0f) / (1 - width);

			//返回阴影颜色
			return std::array<Color, 1>{ {0, 0, 0, power} };
		};

		//计算阴影的顶点
		auto shadowRect = rect;
		//对阴影顶点进行偏移
		for (size_t i = 0;i < shadowRect.size();++i)
			shadowRect[i].SVPosition += Math::Vector4<float>{0.01f, -0.02f, 0, 0};

		//创建像素着色器阶段
		RenderPipeline::PixelShaderStage<decltype(renderTarget), decltype(ShadowPixelShader)>
			ps{ &renderTarget,ShadowPixelShader };

		//创建光栅器
		RenderPipeline::TriangleRasterizer<decltype(ps), float>
			raster{ &ps };

		//创建图元阅读器
		Primitive::PrimitiveListReader<3> reader{ indicis,sizeof(indicis) / sizeof(size_t) };

		//渲染软阴影
		raster.EmitPrimitive(reader, shadowRect.data(), shadowRect.size(), ParallelStrategy::SingleThreadRunner{});
	}

	//字体的像素着色器
	const auto PixelShader = [&tex,mode, width](const Vertex& v,const auto&)
	{
		//从字模纹理创建采样器
		Texture::Sampler::Sampler2D<
			std::remove_reference<decltype(tex)>::type,
			decltype(Texture::Sampler::UVNormalizer::UV2DClamp),
			decltype(Texture::Sampler::Fliters::Bilinear)>
			sampler{
			&tex,
			Texture::Sampler::UVNormalizer::UV2DClamp,
			Texture::Sampler::Fliters::Bilinear};

		const float distance = sampler.Sample(v.UV).x;	//获取与边的距离

		Color col{ 0,0,0,0 };	//基础颜色

		//如果开启了描边
		if (mode == FontMode::Outline)
		{
			//width ~ width + 0.2f
			//描边力度（比笔划粗细度粗0.2）
			float power = (std::clamp(distance, width - 0.2f, width) - (width - 0.2f)) / 0.2f;

			//当前颜色设置为描边后的颜色
			col = { 1, 0.5f, 0.6f, power };
		}

		//根据笔划粗细度计算基本字体的渲染强度
		const float power = std::clamp((distance - width) / 0.05f, 0.0f, 1.0f);

		//将基本字体的颜色混合进col
		col = ExampleCommon::BlendColorNormal(Color{ 0, 0, 0, power }, col);
		
		//提交col颜色
		return std::array<Color, 1>{ col };
	};

	//创建像素着色器阶段
	RenderPipeline::PixelShaderStage<decltype(renderTarget), decltype(PixelShader)>
		ps{ &renderTarget,PixelShader };

	//创建光栅器
	RenderPipeline::TriangleRasterizer<decltype(ps), float>
		raster{ &ps };

	//创建图元阅读器
	Primitive::PrimitiveListReader<3> reader{ indicis,sizeof(indicis) / sizeof(size_t) };

	//渲染基本字体
	raster.EmitPrimitive(reader, rect.data(), rect.size(), ParallelStrategy::SingleThreadRunner{});
}

//绘制一行文字
void DrawText(
	const std::wstring& text,
	const std::map<wchar_t,Math::Vector4<float>>& fontIndicis,
	const Texture::Texture2D<Color>& tex,
	Texture::Texture2D<Color>& renderTarget,
	float startX,float size, FontMode mode,float width)
{
	float x = startX;

	for (auto ch : text)
	{
		const auto uvRect = fontIndicis.at(ch);
		const Math::Vector4<float> pos
		{
			x,-0.5F,size*uvRect.z/2.5F,size*uvRect.w/2
		};

		x += size*uvRect.z / 2.5F;

		DrawChar(pos, uvRect, tex, renderTarget, mode,width);
	}
}

int __cdecl main()
{
	constexpr Math::Vector2<int> ScreenSize{ 640,480 };

	SDL::SDLInstance sdl;
	SDL::Window window
	{ 
		"SDFFont Demo",
		SDL::Rect<int>
		{
			SDL::Window::Center,
			SDL::Window::Center,
			ScreenSize.x,
			ScreenSize.y
		},
		SDL::Window::WindowFlag::Null
	};

	//创建单线程执行策略
	ParallelStrategy::SingleThreadRunner runner;
	
	//创建渲染目标
	Texture::Texture2D<Color> renderTarget{ {640,480} };

	//加载字模
	const auto font = LoadBMP24<Color>("msyh0.bmp",runner);

	//获取字模上各个字符的UV方框
	const auto fontIndicis = LoadFontIndex(font.GetSize());
	
	SDL::Keyboard keyboard;

	//文字大小
	float size = 2;

	//文本最左边文字的x坐标
	float startX = -0.75f;

	//字体的粗细度
	float width = 0.45f;

	//字体的渲染模式
	FontMode mode = FontMode::Normal;
	while (!sdl.QuitRequested())
	{
		//清空渲染目标
		renderTarget.Clear({ 1,1,1,1 }, runner);

		//绘制文字
		DrawText(L"ClosestGL向量场字体渲染", fontIndicis, font, renderTarget, startX, size, mode, width);


		//WS按键调整大小
		if (keyboard.KeyPressed("W"))
			size += 0.01f;
		if (keyboard.KeyPressed("S"))
			size -= 0.01f;

		//AD按键左右平移
		if (keyboard.KeyPressed("A"))
			startX += 0.01f;
		if (keyboard.KeyPressed("D"))
			startX -= 0.01f;

		//QE按键改变字体粗细
		if (keyboard.KeyPressed("Q"))
			width += 0.001f;
		if (keyboard.KeyPressed("E"))
			width -= 0.001f;

		//ZXC按键改变渲染模式
		if (keyboard.KeyPressed("Z"))
			mode = FontMode::Normal;
		if (keyboard.KeyPressed("X"))
			mode = FontMode::Outline;
		if (keyboard.KeyPressed("C"))
			mode = FontMode::Shadow;

		//更新窗口
		UpdateWindow(window, renderTarget);
	}

	return 0;
}
